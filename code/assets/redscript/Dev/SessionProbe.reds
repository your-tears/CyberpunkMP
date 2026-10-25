// Temporary probe for the "multiplayer button" work: logs which developer game definitions,
// worlds and genders the retail build exposes (inkISystemRequestsHandler.StartGameDefinition/StartWorld).
// Remove once a session start method is chosen.

@wrapMethod(SingleplayerMenuGameController)
protected cb func OnInitialize() -> Bool {
    let result = wrappedMethod();
    let handler = this.GetSystemRequestsHandler();

    let categories = handler.GetGameDefCategories();
    CyberpunkMPLog(s"[SessionProbe] gamedef categories: \(ArraySize(categories))");
    let i = 0;
    while i < ArraySize(categories) {
        let definitions = handler.GetGameDefinitions(i);
        CyberpunkMPLog(s"[SessionProbe] category \(i) '\(categories[i])': \(ArraySize(definitions)) definitions");
        for definition in definitions {
            CyberpunkMPLog(s"[SessionProbe]   \(definition)");
        }
        i += 1;
    }

    let worlds = handler.GetWorlds();
    CyberpunkMPLog(s"[SessionProbe] worlds: \(ArraySize(worlds))");
    for world in worlds {
        CyberpunkMPLog(s"[SessionProbe]   \(world)");
    }

    let genders = handler.GetGenders();
    CyberpunkMPLog(s"[SessionProbe] genders: \(ArraySize(genders))");
    for gender in genders {
        CyberpunkMPLog(s"[SessionProbe]   \(gender)");
    }

    return result;
}

// Temporary experiment: start Night City directly through the developer world start,
// without a save, prologue or character creation.
@wrapMethod(SingleplayerMenuGameController)
private func PopulateMenuItemList() -> Void {
    wrappedMethod();
    this.AddMenuItem("МУЛЬТИПЛЕЕР (ТЕСТ)", n"CyberpunkMPStartWorld");
}

@wrapMethod(SingleplayerMenuGameController)
protected func HandleMenuItemActivate(data: ref<PauseMenuListItemData>) -> Bool {
    if Equals(data.eventName, n"CyberpunkMPStartWorld") {
        // indices from the probe log: worlds[0] = 03_night_city, genders[2] = Male
        CyberpunkMPLog("[SessionProbe] StartWorld(0 = 03_night_city, 2 = Male)");
        this.GetSystemRequestsHandler().StartWorld(0, 2);
        return true;
    }
    return wrappedMethod(data);
}
