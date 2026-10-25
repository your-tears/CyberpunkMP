# CLAUDE.md

## Проект

Форк **CyberpunkMP** (Tilted Phoques SRL, https://github.com/tiltedphoques/CyberpunkMP),
мультиплеер-мода для Cyberpunk 2077. Синхронизирует игроков (внешность, экипировка,
движение, анимации) и машины с пассажирами. На его основе мы делаем RP-сервер
(по образцу ранних GTA 5 RP на FiveM).

Upstream заброшен: последний коммит 21.12.2024, поддерживается только патч 2.2.

Общение с пользователем на русском. Пользователь не опытный C++-разработчик, поэтому
объяснять коротко и по делу: что делаем и зачем.

## Текущая цель

- **Этап 1 (сейчас):** обновить под патч игры **2.5** (вышел 22.10.2026), собрать,
  запустить сервер локально, зайти из игры. Ветка `update-2.5`.
- Этап 2: RP-фреймворк в виде **плагинов** (C# через SDK + Redscript): персонажи, БД,
  деньги, инвентарь, работы, фракции. Ядро без необходимости не трогаем.
- Этап 3: собственный лаунчер, ставящий всё в один клик.

## Окружение (проверено 25.10.2026)

- Windows 11, VS 2022 Community 17.14 (MSVC 14.44) в `D:\VS\2022\Community`
- Windows SDK 10.0.22621.0 (**должен быть < 10.0.26100.0**, иначе ломается protobuf-cpp)
- xmake 3.1.2, git 2.55, .NET SDK 8.0, Node 24, pnpm 12
- Если в шелле не находится xmake/node/pnpm: PATH сессии устарел. В PowerShell перечитать:
  `$env:Path = [Environment]::GetEnvironmentVariable('Path','Machine') + ';' + [Environment]::GetEnvironmentVariable('Path','User')`

- NuGet: на этой машине был пустой список источников (ошибка NU1100 при сборке SdkGenerator).
  Исправлено: `dotnet nuget add source https://api.nuget.org/v3/index.json -n nuget.org`
- xmake берёт свежие версии пакетов (`package.requires_lock` = false), а не версии 2024 года.
  При странных ошибках в сторонних библиотеках проверить это первым делом.

- protobuf-cpp зафиксирован на **28.1**: `code/netpack/cpp/helpers.cc` — копия внутреннего
  генератора protobuf, сверено с оригиналом (у 28.x отличие 2 строки). Lock-файл указывает 27.0, он устарел.
- SdkGenerator (CppSharp 1.1.5, старый Clang) не парсил STL из MSVC 14.44 (STL1000).
  Исправлено define `_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH`. Генератор глотает
  исключения, поэтому при ошибках CS0234 `CyberpunkSdk.Internal` запустить его вручную:
  `dotnet run --project code/server/scripting/SdkGenerator/SdkGenerator.csproj -- code/server/native/Scripting/I*.h`

- pnpm 12 падает с ERR_PNPM_IGNORED_BUILDS, если зависимости нужен install-скрипт без
  разрешения. Решение: `pnpm-workspace.yaml` с `allowBuilds: { esbuild: true }` в
  `code/server/admin` и `code/server/scripting/EmoteSystem/Admin`. Лаунчеру понадобится то же (этап 3).

## Структура

```
code/
  client/        C++ RED4ext-плагин (CyberpunkMP.dll), клиентская часть
    Main.cpp       точка входа RED4ext (Main/Query/Supports), регистрация скриптов, ArchiveXL, TweakXL, Input Loader
    App/           логика: сеть, RPC, World (синхронизация игроков/машин, интерполяция), UI
    Core/          хуки (Hooking/, Facades/Hook.hpp), Raw.hpp (функции движка по хешу адреса)
    Red/           встроенная копия RedLib (обёртки над RTTI движка)
    Game/          описания игровых структур
    Support/       провайдеры: MinHook, RED4ext, RedLib, Spdlog, Renderer
  common/        общий C++ код клиента и сервера
  protocol/      .proto-файлы сетевого протокола (protobuf-cpp)
  netpack/       генератор кода сети
  server/
    native/      C++ ядро сервера (GameServer, PlayerManager, Scripting-интерфейсы)
    loader/      C# хост сервера (Server.Loader), грузит native и плагины
    scripting/   .NET SDK (CyberpunkSdk, SdkGenerator) и плагины-примеры:
                 JobSystem (такси/курьер/полиция), EmoteSystem, VehicleSystem (исключён из сборки)
    admin/       веб-админка (React, pnpm)
  assets/
    redscript/   клиентские .reds; Plugins/ = клиентская часть плагинов-примеров
    Archives, Tweaks, Inputs, CET
  launcher/      Electron-лаунчер (pnpm)
  loader/        xmake-цель для запуска игры из VS (нужен `xmake f --game=...`)
tools/           codegen, правило сборки csharp
vendor/          git-подмодули: RED4ext.SDK (форк tiltedphoques), ArchiveXL, TweakXL, Codeware
                 (от последних трёх берутся только заголовки support/red4ext)
```

## Сборка

```
git submodule update --init
xmake -y            # -v для подробного вывода
xmake project -k vsxmake   # решение для Visual Studio в папке vsxmake/
```

xmake собирает всё сразу: C++ клиент и сервер, C# SDK и плагины (dotnet), админку (pnpm).

## Установка и запуск (проверено 25.10.2026)

**Быстрый путь для разработки:** `start-dev.cmd` (логика в `tools/dev/start-dev.ps1`). Находит
игру через Steam (у пользователя `D:\steam\steamapps\common\Cyberpunk 2077`), проверяет версии
зависимостей, делает junction `red4ext/plugins/zzzCyberpunkMP` → `dist/launcher/mod`, запускает
сервер (пароль админки генерируется в `dist/dev-settings.json`) и игру. `-Build`: пересобрать и
переразложить в dist перед запуском. Обычную папку zzzCyberpunkMP (не ссылку) скрипт не трогает.

Раскладка: `xmake install -y -o dist Client redscript Archives Inputs Tweaks Server.Native Server.Scripting Server.Loader Admin`
(путь `-o` только относительный: Admin ломается на абсолютном; полный `xmake install`
тянет Electron-лаунчер, он падает на pnpm 12). Результат: `dist/launcher/mod` (клиент), `dist/launcher/server`.

- Клиент: содержимое `dist/launcher/mod` копируется в `<игра>/red4ext/plugins/zzzCyberpunkMP/`
  (zzz нужно, чтобы грузиться после ArchiveXL и TweakXL). Без флага `--online` мод выключен.
  Запуск: `bin/x64/Cyberpunk2077.exe --online --ip=127.0.0.1` (порт по умолчанию 11778, флаг `--port=`).
- Зависимости под 2.5: RED4ext 1.31.0, Redscript **0.5.32** (стабильная ветка, поддерживает 2.5;
  1.0 пока только preview), ArchiveXL 1.28.0, TweakXL 1.12.0, Codeware 1.21.0,
  Input Loader 0.2.3 (обязателен, Main.cpp без него падает; под 2.5 официально НЕ подтверждён),
  CET 1.38.0 (нужен только для `code/assets/CET/init.lua` плагина работ, автоматически не ставится).
- Сервер: `dist/launcher/server/Server.Loader.exe`, обязательны переменные окружения
  `CYBERPUNKMP_ADMIN_USERNAME` / `CYBERPUNKMP_ADMIN_PASSWORD`. Конфиг `config/server.json`
  (создаётся при первом запуске), логи `logs/`. Порты: игра UDP 11778, веб 11779, Flecs REST 27750.
  «Server could not reach the server list» безвредно (мастер-сервер Tilted Phoques мёртв).

Подключение к серверу делается в мире, а не в меню: загрузить сейв или начать игру, затем
удерживать `/` (UIConnectToServer, Connect() идёт на `--ip`/`--port`). Остальные клавиши из
`code/assets/Inputs/CyberpunkMP.xml`: чат `;`, эмоции `.`, работы `,`, прокрутка чата PageUp/PageDown.
25.10.2026: игра с модом впервые дошла до главного меню на 2.5.

Логи для отладки: `red4ext/logs/`, `red4ext/plugins/zzzCyberpunkMP/CyberpunkMP.log`,
`r6/logs/redscript_rCURRENT.log`, `red4ext/plugins/{ArchiveXL,TweakXL,Codeware}/*.log`,
`bin/x64/plugins/cyber_engine_tweaks/cyber_engine_tweaks.log`, краши: `%LOCALAPPDATA%\REDEngine\ReportQueue\`.

## Как клиент обращается к движку

- Функции движка вызываются через `Core::RawFunc<hash, signature>` (`code/client/Core/Raw.hpp`).
  Адрес ищется по **хешу** через `RED4ext::UniversalRelocBase::Resolve(hash)`, по базе
  адресов, которую поставляет игра. Жёстко прописанных адресов нет, поэтому после патча
  ломается только то, что CDPR удалили или у чего поменяли сигнатуру.
- Хуки: `Core/Hooking`, `Core/Facades/Hook.hpp` (MinHook).
- Структуры движка берутся из RED4ext.SDK (`RED4ext/Scripting/Natives/...`).
- Main.cpp: `RED4EXT_V0_RUNTIME_INDEPENDENT`, `RED4EXT_SDK_LATEST`, `RED4EXT_API_VERSION_LATEST`.

## Лицензия

- Изменения самой платформы публикуем открыто (GitHub) с атрибуцией Tilted Phoques SRL.
  Публиковать на Nexus и других мод-площадках нельзя.
- Плагины через SDK освобождены от этих ограничений. JobSystem, EmoteSystem и
  `code/assets/redscript/Plugins` под MIT.

## Правила работы

- Маленькие шаги, без огромных изменений за раз.
- **Никогда не выдумывать** адреса функций, хеши, смещения структур и сигнатуры движка.
  Если не уверен, сказать прямо и предложить способ проверки (логи, отладчик,
  Ghidra/IDA, RED4ext.SDK, исходники Codeware/CET).
- Claude не видит игру: всё, что требует запуска игры, делает пользователь. Говорить
  точно, что запустить и что принести.
- Перед удалением или большой переделкой файлов спрашивать пользователя.
- Если упираемся в зависимость, не обновлённую под 2.5, сказать и решить вместе: ждать или обходить.
- Только адаптация под новые API, логику мода не переписывать.
- После каждого шага, который собирается, коммит с понятным сообщением.
- В конце шага кратко: что сделано, что дальше, что нужно от пользователя.

## Журнал находок

- 25.10.2026: склонировано, ветка `update-2.5`. Исходные подмодули: ArchiveXL v1.18.1+6,
  Codeware v1.13.0, TweakXL v1.10.6, RED4ext.SDK tiltedphoques@f52af6a.
- 25.10.2026: релизы с поддержкой 2.5: ArchiveXL v1.28.0, TweakXL v1.12.0, Codeware v1.21.0
  (все от 25.10.2026). RED4ext.SDK upstream (wopss) master: «Add support for patch 2.5» (#227)
  и «Update types for patch 2.5» (#232).
- RED4ext.SDK: форк tiltedphoques = upstream@455d9b8 (15.11.2024) + ~35 своих коммитов
  (реверс vehicleBaseObject, moveComponent, entEntity, inkWidget/TextWidget, инвентарь,
  CharacterCustomizationState и др., ~90 файлов). Клиент от них зависит, поэтому план:
  влить upstream/master в форк, а не переходить на чистый upstream.
  **Риск:** смещения полей из реверса форка (под 2.2) в 2.5 могли сдвинуться. Компилятор
  этого не увидит, проверять в рантайме.
- RED4ext.SDK: локальная ветка `cyberpunkmp-2.5` (коммит 17387e0f, ещё не запушена) = форк +
  wopss master@5246e659 (тот же коммит, против которого собран Codeware v1.21.0). Прошлые
  слияния в форке молча откатывали куски upstream. Правило разрешения: ядро SDK и разметка
  структур из upstream; файлы, где форк только удалял код upstream, тоже из upstream; чисто
  добавочные файлы и поля форка сохранены; поля форка внутри структур, чья разметка в 2.5
  поменялась, убраны. Нужен include `RED4ext.SDK/vendor/D3D12MemAlloc` (vendor/xmake.lua).
- В 2.5 изменились размеры: game::Puppet 0x5F8→0x618, Item_Record 0x478→0x488,
  CharacterCustomizationState 0xF0→0x110, ink::TextWidget (+0x30), vehicle::BaseObject
  (сдвиги неравномерные: +0x8…+0x30), WheeledBaseObject 0xBE0→0xC10. move::Component не изменился (0x2C0).
- `code/client/Red/` = RedLib. Было: копия из Codeware v1.6.1 + 4 правки. Заменено на RedLib из
  Codeware v1.21.0, поверх три правки CyberpunkMP: `IsNone` в GetClass,
  `remove_cvref_t` в Invocation, `RED4ext::SharedPtr` в Utils/Resources.hpp (в проекте есть
  глобальный `SharedPtr` = std::shared_ptr). Адреса по хешу в новой RedLib уже есть.
- Проверка всех файлов клиента разом (xmake останавливается на первой ошибке): скрипт
  `checkall.ps1` в scratchpad. Он компилирует каждую команду из compile_commands.json отдельно.

### Реестр полей, зависящих от разметки 2.5 (НЕ проверены)

Решение пользователя: где можно, надёжная замена без офсетов, остальное временно отключить
с `spdlog::warn("[2.5] ...")`. Все места помечены в коде комментарием `Patch 2.5:`.

| Где в клиенте | Что | Статус |
|---|---|---|
| App/Extensions/inkTextInputWidget.cpp | TextInputWidget::caretPosition (2.2: 0x434) | ОТКЛЮЧЕНО (MoveCaret нигде не вызывается) |
| App/World/NetworkWorldSystem.cpp | Puppet::moveComponent (2.2: 0x420) | ЗАМЕНЕНО: поиск move::Component в `components` |
| Game/Animation/MultiMovementController.cpp | move::Component::representation (0x138) | ОТКЛЮЧЕНО (Reset() нигде не вызывается) |
| App/World/NetworkWorldSystem.cpp | vehicle rigidBody->currentTransform | ЗАМЕНЕНО: transformComponent->worldTransform |
| App/World/NetworkWorldSystem.cpp | vehicle rigidBody->velocity | ЗАМЕНЕНО: скорость по смещению позиции / dt |
| App/World/VehicleSystem.cpp | vtable +0x328 («включить двигатель») и engineData->unk61 | ОТКЛЮЧЕНО, чужие машины без запуска двигателя |
| App/World/NetworkWorldSystem.cpp, AppearanceSystem.cpp | isBodyGenderMale | ЗАМЕНЕНО: `IsBodyGenderMale()` через RTTI-вызов; если функции нет, warn и «мужское» |
| App/World/AppearanceSystem.cpp | виртуальные Get{Head,Body,Arms}Customization (слоты vtable 2.2) | ЗАМЕНЕНО: RawFunc по хешам, как в ArchiveXL 1.28 (`..._Get*Appearances1`). Вариант 1/2 проверить в игре |
| App/World/AppearanceSystem.cpp | Item_Record::appearanceName | ЗАМЕНЕНО: `TweakDB::GetValue<CName>(item + ".appearanceName")` |
| vendor: inkWidget.hpp, vehicleBaseObject.hpp (вне конфликта), inkTextWidget.hpp, ICharacterCustomizationState.hpp (vtable) | поля и виртуалы форка с офсетами 2.2 | компилируются, клиентом сейчас не используются; не использовать без проверки |

| Game/ScriptValidation.h (ScriptBundle, ScriptClass, ScriptFunction и т.д.) | скриптовые структуры движка | ЗАМЕНЕНО на разметку Codeware 1.21.0 (коммит b054150f «Support patch 2.5»): база +0x28, ScriptClass +0x20, ScriptBundle +8. Первый краш в игре был здесь |
| Game/ScriptValidation.h | ScriptFunction::flags | ПРОВЕРЕНО В ИГРЕ: 0xF8 (у Codeware 1.21.0 устаревшее 0xC0; в 2.5 после attributes добавлена ещё одна HashMap на 0xB8). Найдено hex-дампом: у всех `static native` функций там 0x04000011 |
| Game/Rendering.h | рендер-контекст (ImGui-оверлей): адрес контекста, devices, pDirectQueue | ЗАМЕНЕНО по CET 1.38.0 (2.5): хеш `CRenderGlobal_InstanceOffset` 1239944840, devices 0xCEBF48, pDirectQueue 0x14104D0. Второй краш в игре был здесь |
| App/World/VehicleSystem.h | нативное имя RTTI-класса | ИСПРАВЛЕНО: `CyberpunkMPVehicleSystem` (в 2.5 у игры свой нативный VehicleSystem) |
| App/Network/Rpc/RpcValidator.cpp | ScriptReport::fillErrors и AddValidationError | ОТКЛЮЧЕНО, ошибки RPC-валидации пишутся в лог мода |

Отладка крашей: `cdbX64 -z <dmp> -y D:\dev\cyber\build\windows\x64\release -c ".ecxr; kn 30; q"`
(WinDbg поставлен через winget; дампы в `%LOCALAPPDATA%\REDEngine\ReportQueue\`).

Подтверждение из ArchiveXL 1.28 (2.5): группы head/body/arms у CharacterCustomizationState
по 0x70/0x80/0x90, как в реверсе форка, то есть начало структуры не сдвинулось.
