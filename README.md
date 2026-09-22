# Llama Chat Module

**🌍 [English version](README.en.md) | Русский**

---

## Что это

`mod-llama-chat` — модуль для AzerothCore + Playerbots, который подключает к игре **внешнюю LLM**
(llama.cpp, Ollama, vLLM, LM Studio, DeepSeek, OpenRouter и любые OpenAI-совместимые серверы).
Боты получают живую речь: личности, память о разговорах, реакцию на события мира и **многомерные
отношения с игроком** (доверие / привязанность / уважение / влечение), которые растут или падают
от того, как игрок общается и что делает.

Модуль общается с LLM **напрямую** — внешний процесс-переводчик (мост) не нужен.

> **Происхождение:** модуль создан **по мотивам** [`mod-ollama-chat`](https://github.com/DustinHendrickson/mod-ollama-chat)
> (автор Dustin Hendrickson) и развивается как отдельный проект **MRafStudio**.
> Вся история исходного модуля сохранена, авторство указано, лицензия — AGPLv3.

---

## Отличия от исходного модуля

| Что | Детали |
|---|---|
| **OpenAI-совместимый транспорт** | `/v1/chat/completions` — llama.cpp, vLLM, LM Studio, DeepSeek, OpenRouter. Мост больше не нужен |
| **Токен авторизации** | Работает в **любом** режиме (`ApiKey` → `Authorization: Bearer`), включая нативный Ollama и llama.cpp с `--api-key` |
| **Русская локализация** | Таблица `mod_llama_chat_personality_templates_locale` (схема `*_locale` по конвенции AzerothCore): базовые строки (enUS) + русские переводы. Язык выбирается по локали получателя |
| **Многомерные отношения** | 4 оси вместо одной шкалы: `trust` / `affection` / `respect` / `attraction` + `mood`. Модуль **читает** их и подмешивает в промпт |
| **Математика отношений — в Lua** | Веса живут в `lua/00_relations_weights.lua` (русские комментарии). Правка весов **не требует пересборки сервера** |
| **Имя и бренд** | `mod-llama-chat`, таблицы `mod_llama_chat_*`, ключи `mod_llama_chat.*`, команды `.llama` |

---

## Требования

* AzerothCore с модулем **Playerbots** ([mod-playerbots](https://github.com/liyunfan1223/mod-playerbots))
* Движок **ALE** (Lua) — [mod-ale](https://github.com/azerothcore/mod-ale) с `LUA_VERSION=luajit`
  (нужен для отношений, интентов и боевого слоя)
* Зависимости (уже в комплекте с модулем): `nlohmann/json`, `cpp-httplib`; `fmt` берётся из ядра
* Любой LLM-сервер: **llama.cpp** (`llama-server`), **Ollama**, vLLM, LM Studio, DeepSeek, OpenRouter

---

## Установка

```bash
cd /path/to/azerothcore/modules
git clone https://github.com/MRafStudio/mod-llama-chat.git
```

Затем пересобрать ядро и выполнить **установку** (install копирует не только бинарники, но и
Lua-скрипты модуля):

```bash
cmake --build build --config RelWithDebInfo --target install
```

Конфиг: скопировать `conf/mod_llama_chat.conf.dist` в `configs/modules/mod_llama_chat.conf`
и поправить под себя.

**Lua-скрипты устанавливаются автоматически** в `<bin>/lua_scripts/mod-llama-chat/`
(каталог назван по имени модуля). Движок ALE загружает `lua_scripts/` рекурсивно.

---

## Настройка подключения к LLM

### Вариант 1: llama.cpp (рекомендуется)

```ini
mod_llama_chat.Url = http://127.0.0.1:8101/v1/chat/completions
mod_llama_chat.ApiMode = "openai"
mod_llama_chat.ApiKey = ""
mod_llama_chat.Model = "Qwen3.8-27B"
mod_llama_chat.OpenAiDisableThinking = true
```

`OpenAiDisableThinking = true` обязателен для reasoning-моделей (Qwen3 и подобных): без него ответ
уходит в невидимое поле `reasoning_content`, и в чат приходит пустота.

Если llama.cpp запущен с `--api-key`, впишите токен в `mod_llama_chat.ApiKey`.

### Вариант 2: Ollama (нативный API)

```ini
mod_llama_chat.Url = http://127.0.0.1:11434/api/generate
mod_llama_chat.ApiMode = "ollama"
mod_llama_chat.ApiKey = ""
mod_llama_chat.Model = "llama3.2:1b"
```

### Вариант 3: облако (DeepSeek, OpenRouter и др.)

```ini
mod_llama_chat.Url = https://api.deepseek.com/v1/chat/completions
mod_llama_chat.ApiMode = "openai"
mod_llama_chat.ApiKey = "ваш-токен"
mod_llama_chat.Model = "deepseek-chat"
```

### Токен авторизации

`mod_llama_chat.ApiKey` работает **во всех режимах** — и в OpenAI-совместимом, и в нативном Ollama.
Токен уходит заголовком `Authorization: Bearer <токен>`.

---

## Команды (в игре — `.llama`, в консоли — `llama`)

| Команда | Что делает |
|---|---|
| `.llama reload` | Перечитать конфиг, личности и данные без рестарта сервера |
| `.llama status` | Состояние: endpoint, модель, think-режим, очередь, воркеры |
| `.llama test <текст>` | Сквозной тест: что ушло в модель и что пришло |
| `.llama sentiment view [бот] [игрок]` | Показать шкалу отношений |
| `.llama sentiment set <бот> <игрок> <0..1>` | Задать шкалу вручную |
| `.llama personality get\|set\|list` | Личности ботов |

Все команды требуют **SEC_ADMINISTRATOR** (GM level 3+).

---

## Lua-слой: отношения, интенты, бой

Скрипты лежат в модуле (`lua/`) и ставятся в `lua_scripts/mod-llama-chat/`. Правки подхватываются
на живом сервере за пару секунд — **пересборка не нужна**.

| Файл | Назначение |
|---|---|
| `00_relations_weights.lua` | **Веса отношений** с русскими комментариями — правьте цифры и сохраняйте |
| `10_relations.lua` | Считает 4 оси по тону общения и событиям, пишет в таблицу модуля |
| `20_intents.lua` | Русские фразы игрока → действия бота («подъедь ко мне», «стой», «гуляй») |
| `30_audit.lua` | Глушит служебные команды `.server` / `.account` |
| `40_combat.lua` | Боевой слой: спутник держится рядом, прикрывает мастера (тик 500 мс) |

### Как работают отношения (один контур)

```
игрок пишет в чат
      ↓
Lua ловит сообщение (ALE, world-thread)
      ↓ считает по весам из 00_relations_weights.lua
      ↓
пишет в таблицу модуля: mod_llama_chat_bot_player_sentiments
      ↓
модуль читает СВОЮ же таблицу и подмешивает чувства в промпт
      ↓
LLM отвечает «в характере» — с учётом доверия, привязанности, уважения и влечения
```

Пример весов (файл `00_relations_weights.lua`):

```lua
W.разговор_тёплый = { trust = 1, affection = 2, respect = 1, attraction = 1, mood = "рада общению" }
W.разговор_грубый = { trust = -2, affection = -3, respect = -1, attraction = -1, mood = "обижена" }
W.подарок         = { trust = 3, affection = 5, respect = 2, attraction = 2, mood = "рада подарку" }
W.помощь_в_бою    = { trust = 6, affection = 3, respect = 6, attraction = 2, mood = "воодушевлена" }
W.воскрешение     = { trust = 8, affection = 6, respect = 5, attraction = 1, mood = "в долгу" }
```

---

## Локализация

Базовые тексты личностей (enUS) — в таблице `mod_llama_chat_personality_templates`,
переводы — в `mod_llama_chat_personality_templates_locale` (схема `*_locale` по конвенции
AzerothCore: `Locale VARCHAR(4)`, PK `(key, Locale)`). Сейчас добавлены **русские** переводы
всех 33 личностей. Модуль выбирает язык по локали **получателя**: игрок с русским клиентом
получает русскую личность, англоязычный — английскую.

---

## Отключение штатной болтовни Playerbots

Чтобы шаблонные фразы ботов не смешивались с LLM-речью, в `playerbots.conf`:

```ini
AiPlayerbot.EnableBroadcasts = 0
AiPlayerbot.RandomBotTalk = 0
AiPlayerbot.RandomBotEmote = 0
AiPlayerbot.RandomBotSuggestDungeons = 0
AiPlayerbot.EnableGreet = 0
AiPlayerbot.GuildFeedback = 0
AiPlayerbot.RandomBotSayWithoutMaster = 0
```

---

## Лицензия

**AGPLv3** — GNU Affero General Public License v3, см. файл [`LICENSE`](LICENSE).

Модуль является производной работой от `mod-ollama-chat` (Dustin Hendrickson, AGPLv3),
поэтому распространяется под той же лицензией, как того требует copyleft.

---

## Происхождение и благодарности

**Исходный модуль:** [`mod-ollama-chat`](https://github.com/DustinHendrickson/mod-ollama-chat) —
разработан **Dustin Hendrickson** (177 коммитов) при участии Brandyman126, kadeshar, jimm0thy,
Jered, Fiery, Frederick, Ivan Novokhatski, mrdeath5493. Их работа — основа этого проекта,
и она полностью сохранена в истории git.

**Этот проект (`mod-llama-chat`)** развивает **MRafStudio**.

Замечания, отчёты об ошибках и предложения приветствуются.
