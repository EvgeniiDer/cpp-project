#include"WindowManager.h"
#include"ActionManager.h"

ActionManager::ActionManager(WindowManager* windowManager, QObject* parent /* = nullptr */)
	: QObject(parent), m_windowManager(windowManager)
{
	initRegistration();
}

void ActionManager::registerActionFactory(const QString& id, ActionFactory factory)
{
	m_factories[id] = factory;
}

QAction* ActionManager::getAction(const QString& id)
{
	if (m_actions.contains(id))
	{
		return m_actions[id];
	}
	if (m_factories.contains(id))
	{
		QAction* act = m_factories[id](m_windowManager, this);
		m_actions[id] = act;
		return act;
	}
	return nullptr;
}
void ActionManager::initRegistration()
{
	registerActionFactory("File.NewChart", [](WindowManager* wm, QObject* p)
		{
			QAction* act = new QAction(QObject::tr("New Chart"), p);
			act->setShortcut(QKeySequence("Ctrl+N"));
			connect(act, &QAction::triggered, [wm]()
				{
					wm->createWindow("Chart");
				});
			return act;
		});
	registerActionFactory("Tools.ThemeEditor", [](WindowManager* wm, QObject* p)
		{
			QAction* act = new QAction(QObject::tr("Theme Editor"), p);
			connect(act, &QAction::triggered, [wm]()
				{
					wm->createWindow("ThemeEditor");
				});
			return act;
		});
	registerActionFactory("Tools.Properties", [](WindowManager* wm, QObject* p)
	{
		QAction* act = new QAction(QObject::tr("Properties"), p);
		connect(act, &QAction::triggered, [wm]()
			{
				wm->createWindow("Properties");
			});
			return act;
		});

}