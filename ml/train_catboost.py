#!/usr/bin/env python3
"""
Обучение CatBoost на признаках стакана (spoofing detection).

Читает все dataset_test_*.csv, собранные DatasetTestCollector, и обучает
ДВЕ отдельные модели: одну для детекта спуфинга на bid, другую — на ask.
Причина разделения: bid- и ask-спуфинг — разные по смыслу паттерны
(ложная поддержка снизу vs. ложное сопротивление сверху), и объединение
их в один флаг через max() теряло бы информацию о том, с какой стороны
стакана было подозрение.

Запуск (из WSL, с активированным venv):
    python3 train_catboost.py
"""

import argparse
import glob
import os
import re
import sys

import pandas as pd
from catboost import CatBoostClassifier, Pool
from sklearn.metrics import accuracy_score, roc_auc_score, log_loss, classification_report

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

FEATURE_COLUMNS = [
    "spread",
    "imbalance_l1", "imbalance_sma3",
    "vol_depth_bids", "vol_depth_asks",
    "day_session",   # день недели + укрупнённый торговый сегмент суток
]
CATEGORICAL_COLUMNS = ["day_session"]

# Границы торговых сессий по биржам. Для ALOR — заглушка,
# заполнить реальными часами торгов MOEX, когда появятся данные с коннектора.
SESSION_BOUNDARIES = {
    "bybit": [
        (6, 12, "morning"),
        (12, 18, "afternoon"),
        (18, 24, "evening"),
        (0, 6, "night"),
    ],
    "alor": [
        # TODO: заполнить реальными часами сессий MOEX
    ],
}


def hour_to_session(hour: int, boundaries: list) -> str:
    """Группирует час в укрупнённый торговый сегмент суток."""
    for start, end, label in boundaries:
        if start <= hour < end:
            return label
    return "unknown"


SPOOF_THRESHOLD = 0.5  # распределение fake_ratio бимодальное (0.0 или 1.0), порог не критичен

# Описание двух независимых задач: (исходная колонка-эвристика в CSV,
# имя таргета, имя файла модели на выходе).
SPOOF_SIDES = {
    "bid": {
        "source_column": "bid_fake_ratio",
        "target_column": "is_bid_spoof", 
        "model_filename": "catboost_model_bid.cbm",
    },
    "ask": {
        "source_column": "ask_fake_ratio",
        "target_column": "is_ask_spoof",
        "model_filename": "catboost_model_ask.cbm",
    },
}


def file_sort_key(path: str) -> int:
    """dataset_test_N.csv -> N, чтобы восстановить хронологический порядок
    (в CSV нет сырого timestamp — только производные hour/day_of_week)."""
    m = re.search(r"(\d+)(?=\.csv$)", os.path.basename(path))
    return int(m.group(1)) if m else 0


def load_dataset(data_dir: str, pattern: str) -> pd.DataFrame:
    paths = sorted(glob.glob(os.path.join(data_dir, pattern)), key=file_sort_key)
    if not paths:
        sys.exit(f"[train] Не найдено ни одного файла по маске {pattern} в {data_dir}")

    frames = []
    for p in paths:
        try:
            df = pd.read_csv(p)
        except pd.errors.EmptyDataError:
            continue
        df["__source_file__"] = os.path.basename(p)
        frames.append(df)

    full = pd.concat(frames, ignore_index=True)
    print(f"[train] Загружено файлов: {len(paths)}, строк всего: {len(full)}")
    return full


def chronological_split(df: pd.DataFrame, test_size: float):
    """Делит по номеру файла (а не случайно): train — из более ранних
    данных, test — из более поздних. Снижает утечку из-за overlapping
    labels (соседние строки используют пересекающиеся будущие окна)."""
    files_in_order = df["__source_file__"].unique()
    split_at = int(len(files_in_order) * (1 - test_size))
    train_files = set(files_in_order[:split_at])
    test_files = set(files_in_order[split_at:])

    train_df = df[df["__source_file__"].isin(train_files)]
    test_df = df[df["__source_file__"].isin(test_files)]
    return train_df, test_df


def train_one_side(side_name: str, side_cfg: dict, train_df: pd.DataFrame,
                    test_df: pd.DataFrame, args) -> None:
    """Обучает и сохраняет одну модель — либо для bid, либо для ask."""
    target_column = side_cfg["target_column"]

    print(f"\n{'=' * 60}")
    print(f"[train:{side_name}] === Обучение модели: {target_column} ===")
    print(f"{'=' * 60}")

    print(f"[train:{side_name}] Баланс классов (train):")
    print(train_df[target_column].value_counts(normalize=True))

    train_pool = Pool(
        data=train_df[FEATURE_COLUMNS],
        label=train_df[target_column],
        cat_features=CATEGORICAL_COLUMNS,
    )
    test_pool = Pool(
        data=test_df[FEATURE_COLUMNS],
        label=test_df[target_column],
        cat_features=CATEGORICAL_COLUMNS,
    )

    model = CatBoostClassifier(
        iterations=args.iterations,
        learning_rate=args.learning_rate,
        depth=args.depth,
        loss_function="Logloss",
        eval_metric="AUC",
        auto_class_weights="Balanced",  # спуфинг — редкий класс (~10-15% строк)
        random_seed=42,
        verbose=50,
    )
    model.fit(train_pool, eval_set=test_pool, use_best_model=True)

    preds = model.predict(test_pool)
    proba = model.predict_proba(test_pool)[:, 1]

    print(f"\n[train:{side_name}] === Оценка на отложенных (более новых) данных ===")
    print(f"Accuracy: {accuracy_score(test_df[target_column], preds):.4f}")
    try:
        print(f"ROC AUC:  {roc_auc_score(test_df[target_column], proba):.4f}")
    except ValueError as e:
        print(f"ROC AUC:  не посчитан ({e}) — вероятно, в test только один класс")
    print(f"LogLoss:  {log_loss(test_df[target_column], proba):.4f}")
    print(classification_report(test_df[target_column], preds))

    print(f"[train:{side_name}] Важность признаков:")
    importances = model.get_feature_importance(train_pool)
    for name, imp in sorted(zip(FEATURE_COLUMNS, importances), key=lambda x: -x[1]):
        print(f"  {name:<18} {imp:.2f}")

    output_path = os.path.join(args.output_dir, side_cfg["model_filename"])
    os.makedirs(args.output_dir, exist_ok=True)
    model.save_model(output_path)
    print(f"[train:{side_name}] Модель сохранена: {output_path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--data-dir",
        default=os.path.join(SCRIPT_DIR, "..", "out", "build", "windows-x64-debug"),
        help="Папка с dataset_test_*.csv",
    )
    parser.add_argument("--pattern", default="dataset_test_*.csv")
    parser.add_argument(
        "--output-dir",
        default=os.path.join(SCRIPT_DIR, "models"),
        help="Куда сохранить обученные модели (.cbm)",
    )
    parser.add_argument("--test-size", type=float, default=0.2)
    parser.add_argument("--iterations", type=int, default=500)
    parser.add_argument("--learning-rate", type=float, default=0.05)
    parser.add_argument("--depth", type=int, default=6)
    parser.add_argument(
        "--exchange",
        choices=list(SESSION_BOUNDARIES.keys()),
        default="bybit",
        help="Биржа-источник данных — определяет границы торговых сессий",
    )
    args = parser.parse_args()

    df = load_dataset(args.data_dir, args.pattern)

    boundaries = SESSION_BOUNDARIES[args.exchange]
    df["session"] = df["hour"].apply(lambda h: hour_to_session(h, boundaries))
    df["day_session"] = df["day_of_week"].astype(str) + "_" + df["session"]

    source_columns = [cfg["source_column"] for cfg in SPOOF_SIDES.values()]
    required = FEATURE_COLUMNS + source_columns
    missing = [c for c in required if c not in df.columns]
    if missing:
        sys.exit(f"[train] В CSV не хватает колонок: {missing}. "
                  f"Проверь, что схема совпадает с OrderBookCsvExporter::rowToCsvLine.")

    # Строим оба таргета из уже посчитанной в C++ эвристики (по отдельности
    # для bid и ask), а не из будущей цены и не через max() — так каждая
    # модель учится узнавать спуфинг именно на своей стороне стакана.
    for side_cfg in SPOOF_SIDES.values():
        df[side_cfg["target_column"]] = (
            df[side_cfg["source_column"]] >= SPOOF_THRESHOLD
        ).astype(int)

    target_columns = [cfg["target_column"] for cfg in SPOOF_SIDES.values()]
    before = len(df)
    df = df.dropna(subset=FEATURE_COLUMNS + target_columns)
    if len(df) < before:
        print(f"[train] Отброшено строк с пропусками: {before - len(df)}")

    train_df, test_df = chronological_split(df, args.test_size)
    print(f"[train] train: {len(train_df)} строк, test: {len(test_df)} строк")

    if len(train_df) == 0 or len(test_df) == 0:
        sys.exit("[train] Слишком мало данных для разбиения train/test — собери больше CSV.")

    for side_name, side_cfg in SPOOF_SIDES.items():
        train_one_side(side_name, side_cfg, train_df, test_df, args)


if __name__ == "__main__":
    main()
