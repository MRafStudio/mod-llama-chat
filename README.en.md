# Llama Chat Module

**🌍 English | [Русская версия](README.md)**

---

## What it is

`mod-llama-chat` is an **AzerothCore + Playerbots** module that connects the game to an **external LLM**
(llama.cpp, Ollama, vLLM, LM Studio, DeepSeek, OpenRouter and any OpenAI-compatible server).
Bots get living speech: personalities, chat memory, reactions to world events and **multi-axis
relationships with the player** (trust / affection / respect / attraction) that grow or fall
depending on how the player talks and acts.

The module talks to the LLM **directly** — no external translator/bridge process is required.

> **Origin:** this module is **based on** [`mod-ollama-chat`](https://github.com/DustinHendrickson/mod-ollama-chat)
> by Dustin Hendrickson and is maintained as a separate project by **MRafStudio**.
> The full history of the original module is preserved, authorship is credited, license is AGPLv3.

---

## Differences from the original module

| Item | Details |
|---|---|
| **OpenAI-compatible transport** | `/v1/chat/completions` — llama.cpp, vLLM, LM Studio, DeepSeek, OpenRouter. The bridge is no longer needed |
| **Authorization token** | Works in **every** mode (`ApiKey` → `Authorization: Bearer`), including native Ollama and llama.cpp with `--api-key` |
| **Russian localization** | Table `mod_llama_chat_personality_templates_locale` (AzerothCore `*_locale` convention): base strings (enUS) + Russian translations. Language is chosen by the recipient's locale |
| **Multi-axis relationships** | Four axes instead of a single scale: `trust` / `affection` / `respect` / `attraction` + `mood`. The module **reads** them and injects them into the prompt |
| **Relationship math lives in Lua** | Weights are in `lua/00_relations_weights.lua`. Editing them **does not require a server rebuild** |
| **Name and branding** | `mod-llama-chat`, tables `mod_llama_chat_*`, config keys `mod_llama_chat.*`, commands `.llama` |

---

## How it works

### When a bot replies

| Situation | Behaviour |
|---|---|
| **Whisper** (`/w`) | Replies **always**, distance does not matter — if `EnableWhisperReplies = 1` |
| **Say** (`/s`) | Only when the player is within `SayDistance` (default **30 yards**) |
| **Yell** (`/y`) | Only when the player is within `YellDistance` (default **100 yards**) |
| **Party / raid / guild / channels** | Controlled by `Enable*Channel*` settings |
| **In combat** | Depends on `DisableRepliesInCombat`: `0` — replies, `1` — stays silent |
| **No real player nearby** | The bot stays silent — random chatter only fires within `RandomChatterRealPlayerDistance` (default 200 yards) |

> In this project replies in combat are enabled (`DisableRepliesInCombat = 0`) and whisper replies
> are on (`EnableWhisperReplies = 1`), so the bot never goes mute when you need it.

### World events the bot reacts to

The module subscribes to events and may comment on them in character (using its personality):

| Event | What it comments on |
|---|---|
| Creature / player kill | "took down the enemy" |
| Item received | rare loot, a find |
| Player death | sympathy or mockery (per personality) |
| Quest completed | "quest turned in" |
| Spell learned | "learned a new skill" |
| Duel | challenge, outcome |
| Level up | congratulations |
| Achievement | pride for the player |
| GameObject used | reaction to chests/doors/devices |
| Guild events, login | greeting, small talk |
| **Player emote** (wave, hug) | gesture back or a worded reply |

### Where the request goes

```
prompt (on the world thread)
   ↓ into a bounded queue (MaxQueueDepth - overload protection)
worker threads (WorkerThreads) → HTTP
   ↓
ApiMode = "openai" → POST /v1/chat/completions  (+ Authorization: Bearer)
ApiMode = "ollama" → POST /api/generate          (+ Authorization: Bearer)
   ↓ the reply returns to the world thread
emote tags are stripped → gesture plays → text goes to chat as the bot
```

Game objects (Player, Group, Guild, Map) are touched **only on the world thread**; HTTP and string
parsing happen on workers. This is an AzerothCore requirement and the module follows it.

---

## Requirements

* AzerothCore with the **Playerbots** module ([mod-playerbots](https://github.com/liyunfan1223/mod-playerbots))
* **ALE** Lua engine — [mod-ale](https://github.com/azerothcore/mod-ale) built with `LUA_VERSION=luajit`
  (needed for relationships, intents and the combat layer)
* Dependencies (bundled with the module): `nlohmann/json`, `cpp-httplib`; `fmt` comes from the core
* Any LLM server: **llama.cpp** (`llama-server`), **Ollama**, vLLM, LM Studio, DeepSeek, OpenRouter

---

## Installation

```bash
cd /path/to/azerothcore/modules
git clone https://github.com/MRafStudio/mod-llama-chat.git
```

Then rebuild the core and run **install** (install copies the binaries *and* the module's Lua scripts):

```bash
cmake --build build --config RelWithDebInfo --target install
```

Configuration: copy `conf/mod_llama_chat.conf.dist` to `configs/modules/mod_llama_chat.conf`
and adjust it.

**Lua scripts are installed automatically** into `<bin>/lua_scripts/mod-llama-chat/`
(the folder is named after the module). ALE loads `lua_scripts/` recursively.

---

## Connecting to an LLM

### Option 1: llama.cpp (recommended)

```ini
mod_llama_chat.Url = http://127.0.0.1:8101/v1/chat/completions
mod_llama_chat.ApiMode = "openai"
mod_llama_chat.ApiKey = ""
mod_llama_chat.Model = "Qwen3.8-27B"
mod_llama_chat.OpenAiDisableThinking = true
```

`OpenAiDisableThinking = true` is required for reasoning models (Qwen3 and similar): without it the
answer goes into the invisible `reasoning_content` field and the chat receives nothing.

If llama.cpp runs with `--api-key`, put the token into `mod_llama_chat.ApiKey`.

### Option 2: Ollama (native API)

```ini
mod_llama_chat.Url = http://127.0.0.1:11434/api/generate
mod_llama_chat.ApiMode = "ollama"
mod_llama_chat.ApiKey = ""
mod_llama_chat.Model = "llama3.2:1b"
```

### Option 3: cloud (DeepSeek, OpenRouter, etc.)

```ini
mod_llama_chat.Url = https://api.deepseek.com/v1/chat/completions
mod_llama_chat.ApiMode = "openai"
mod_llama_chat.ApiKey = "your-token"
mod_llama_chat.Model = "deepseek-chat"
```

### Authorization token

`mod_llama_chat.ApiKey` works in **all** modes — both OpenAI-compatible and native Ollama.
The token is sent as the `Authorization: Bearer <token>` header.

---

## Commands (in game `.llama`, in console `llama`)

| Command | What it does |
|---|---|
| `.llama reload` | Reload config, personalities and data without restarting the server |
| `.llama status` | State: endpoint, model, think mode, queue, workers |
| `.llama test <text>` | End-to-end test: what was sent to the model and what came back |
| `.llama sentiment view [bot] [player]` | Show the relationship scale |
| `.llama sentiment set <bot> <player> <0..1>` | Set the scale manually |
| `.llama personality get\|set\|list` | Bot personalities |

All commands require **SEC_ADMINISTRATOR** (GM level 3+).

---

## Lua layer: relationships, intents, combat

Scripts live in the module (`lua/`) and are installed into `lua_scripts/mod-llama-chat/`.
Edits are picked up on a live server within seconds — **no rebuild required**.

| File | Purpose |
|---|---|
| `00_relations_weights.lua` | **Relationship weights** with Russian comments — change the numbers and save |
| `10_relations.lua` | Computes the four axes from chat tone and events, writes them into the module table |
| `20_intents.lua` | Russian player phrases → bot actions ("come to me", "stay", "go grind") |
| `30_audit.lua` | Silences service commands `.server` / `.account` |
| `40_combat.lua` | Combat layer: companion stays close, covers the master (500 ms tick) |

### How relationships work (single loop)

```
player writes in chat
      ↓
Lua catches the message (ALE, world thread)
      ↓ computes using weights from 00_relations_weights.lua
      ↓
writes into the module table: mod_llama_chat_bot_player_sentiments
      ↓
the module reads its own table and injects feelings into the prompt
      ↓
the LLM answers "in character" — aware of trust, affection, respect and attraction
```

Example weights (file `00_relations_weights.lua`):

```lua
W.warm_talk   = { trust = 1, affection = 2, respect = 1, attraction = 1, mood = "happy to talk" }
W.rude_talk   = { trust = -2, affection = -3, respect = -1, attraction = -1, mood = "offended" }
W.gift        = { trust = 3, affection = 5, respect = 2, attraction = 2, mood = "pleased with the gift" }
W.help_in_fight = { trust = 6, affection = 3, respect = 6, attraction = 2, mood = "inspired" }
W.resurrection  = { trust = 8, affection = 6, respect = 5, attraction = 1, mood = "in your debt" }
```

---

### Relationship scale (fractional)

Axes are measured in **tenths**: `0.00 … 100.00` (type `DECIMAL(5,2)`). Integers are too coarse —
with a rough scale three warm phrases were enough to become a "best buddy". Friendship is earned now.

| Range | What the bot thinks of you |
|---|---|
| 0–20 | hostile |
| 21–40 | distrustful |
| 41–60 | neutral |
| 61–80 | **friend** (trust ≥ 60 and affection ≥ 60) |
| 81–100 | close |

Thresholds live in `ПОРОГИ` inside the weights file. Respect does **not** participate in the friendship
check — you can respect an enemy.

### Weights (file `lua/00_relations_weights.lua`)

| Event | Gain (example) |
|---|---|
| Ordinary chat | +0.02 … +0.03 |
| Warm phrase ("thanks", "well done") | +0.10 … +0.20 |
| Rudeness | −0.60 … −0.80 (**three times heavier than warmth**) |
| Gift | +0.80 … +1.50 |
| Help in combat | +1.50 (trust and respect) |
| Resurrection | +2.50 |
| Hug / kiss | +0.30 … +0.40 |
| **Sparring with a friend** | **+0.05 … +0.10** |

Scale is tuned so that going from neutral 50 to the friendship threshold takes ~200 warm phrases,
**or** ~60 gifts, **or** ~30 fights helped. Ruining a relationship is three times faster than fixing it.

### Duels: sparring is not aggression

A fight between friends is **training**. Duels are handled separately from attacks:

| Situation | Reaction |
|---|---|
| Duel with a **friend** | "let's warm up!" + tiny skill gain (`спарринг_друг`) |
| Duel with anyone else | even smaller (`спарринг`) |
| Bot lost to a friend | a little respect to the opponent, no grudge |
| Bot beat the player | just a line — beating your master does **not** raise his trust |
| **Attacking outside a duel** | **betrayal**: −5.00 trust, −3.00 affection |

## Localization

Base personality strings (enUS) are in `mod_llama_chat_personality_templates`, translations are in
`mod_llama_chat_personality_templates_locale` (AzerothCore `*_locale` convention: `Locale VARCHAR(4)`,
PK `(key, Locale)`). Russian translations for all 33 personalities are included. The module picks the
language by the **recipient's** locale.

---

## Disabling the stock Playerbots chatter

To keep template phrases from mixing with LLM speech, set in `playerbots.conf`:

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

## License

**AGPLv3** — GNU Affero General Public License v3, see [`LICENSE`](LICENSE).

This module is a derivative work of `mod-ollama-chat` (Dustin Hendrickson, AGPLv3) and is therefore
distributed under the same license, as copyleft requires.

---

## Origin and credits

**Original module:** [`mod-ollama-chat`](https://github.com/DustinHendrickson/mod-ollama-chat) —
developed by **Dustin Hendrickson** (177 commits) with contributions from Brandyman126, kadeshar,
jimm0thy, Jered, Fiery, Frederick, Ivan Novokhatski, mrdeath5493. Their work is the foundation of
this project and is fully preserved in the git history.

**This project (`mod-llama-chat`)** is maintained by **MRafStudio**.

Issues, bug reports and suggestions are welcome.
