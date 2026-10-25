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
