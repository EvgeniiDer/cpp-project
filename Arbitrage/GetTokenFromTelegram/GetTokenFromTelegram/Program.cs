using System;
using System.Net.Http;
using System.Threading.Tasks;
using System.Text;
using System.Text.Json;
using TL;
using WTelegram;
/*
Что делает этот код:
Он использует библиотеку WTelegram для взаимодействия с Telegram API.
Заходит в ваш Telegram-аккаунт.
Находит нужного бота (tapper_tgmrkt_bot).
Нажимает кнопку /start, чтобы получить кнопку запуска Web App.
"Считывает" специальную ссылку из этой кнопки. В этой ссылке Telegram прячет уникальные данные (initData), которые доказывают, что именно ваш аккаунт запускает это приложение.
Отправляет эти initData на сервер приложения (api.tgmrkt.io).
Сервер проверяет initData, убеждается, что они настоящие, и в ответ выдает временный токен авторизации.
Код печатает этот токен, который вы потом можете использовать для других запросов к серверу игры/сервиса.
*/
//App api_id: 29394492
//APP api_hash: e7b3fcc50b2b13ab987c4122cb13af87
class Program
{
    static int API_ID = 29394492; // Replace with your API ID
    static string API_HASH = "e7b3fcc50b2b13ab987c4122cb13af87"; // Replace with your API hash 
    static string BOT_USERNAME = "tapper_tgmrkt_bot"; // Replace with your bot 
    static string AUTH_TOKEN = "https://api.tgmrkt.io/api/v1/auth";

    static async Task Main(string[] args)
    {
        try
        {
            using (WTelegram.Client client = new WTelegram.Client(Config))
            {
                var user = await client.LoginUserIfNeeded();
            }
            ;

        }
        catch (Exception e)
        {
            Console.WriteLine($"Error: {e.Message}");
            return;
        }
    }

}

