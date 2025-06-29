using System;
using System.Net.Http;
using System.Threading.Tasks;
using System.Text;
using System.Text.Json;
using TL;
using WTelegram;
using System.Runtime.CompilerServices;
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
    static int API_ID = 29394492; 
    static string API_HASH = "e7b3fcc50b2b13ab987c4122cb13af87"; 
    static string BOT_USERNAME = "MRKT"; // Replace with your bot 
    static string AUTH_TOKEN = "https://api.tgmrkt.io/api/v1/auth";

   private static readonly HttpClient httpCleitn = new HttpClient();

    static async Task Main(string[] args)
    {
        Console.WriteLine("Begining Auth in Telegram Web App...");
        
        using Client client = new Client(Config);
        try
            {
            TL.User user = await client.LoginUserIfNeeded();
            Console.WriteLine($"Successfully logged in as: {user.first_name} {user.last_name}");
            Console.WriteLine("Next");
            var resolvedPeer = await client.Contacts_ResolveUsername(BOT_USERNAME);
            if (resolvedPeer.User == null)
                {
                Console.WriteLine($"Can't find@{BOT_USERNAME}. ID: {resolvedPeer.User.ID}");
                }

            Console.WriteLine($"Bot @{BOT_USERNAME} finded. ID: {resolvedPeer.User.ID}");

            TL.User botUser = resolvedPeer.User;
            Console.WriteLine("-----------------------------------------------------");
            TL.Messages_MessagesBase history = await client.Messages_GetHistory(botUser, limit: 1);
            TL.Message lastMessage = history.Messages.OfType<Message>().FirstOrDefault();
            Console.WriteLine("Last message: " + lastMessage);

            if (lastMessage.reply_markup is not ReplyInlineMarkup inlineMarkup)
                {
                Console.WriteLine("No inline keyboard found in the last message.");
                return;
                }
            KeyboardButton webViewButton = null;
            foreach (TL.KeyboardButtonRow row in inlineMarkup.rows)
                {
                foreach (KeyboardButton button in row.buttons)
                    {
                    if (button is KeyboardButtonWebView || button is KeyboardButtonUrl)
                        {
                        webViewButton = button;
                        break;
                        }
                    }
                if (webViewButton != null)
                    {
                    break;
                    }
                }
            Console.WriteLine("KeyboardButtonview: " + webViewButton.Text);
            string buttonUrl = null;
            if(webViewButton is KeyboardButtonWebView urlButton)
                {
                buttonUrl = urlButton.url;

                Console.WriteLine("Url: " + buttonUrl);
                }
            else if(string.IsNullOrEmpty(buttonUrl))
                {
                Console.WriteLine("Failed to extract URL from button.");
                return;
                }
            Console.WriteLine("----------------------------------------------");

            Console.WriteLine("Simulate a button click (RequestWebView request)...");

            WebViewResult webViewResult = await client.Messages_RequestWebView(
                peer: botUser,
                bot: botUser,
                url: buttonUrl,
                platform: "android"
                );
            Console.WriteLine("Received authorization link from Telegram!");
            }
        catch (Exception ex)
                {
                Console.WriteLine(ex.Message);
                }
     }
        private static string Config(string what)
        {
            switch(what)
            {
            case "api_id":
                return API_ID.ToString();
            case "api_hash":
                return API_HASH;
            case "phone_number":
                return "+79268766084";
            case "session_pathname":
                return "wtsession.dat";
            case "verification_code":
            case "password":
            Console.Write($"Введите ваш {what.Replace('_', ' ')}: ");
            return Console.ReadLine();
            default: return null;
            }
        }
    }
   



