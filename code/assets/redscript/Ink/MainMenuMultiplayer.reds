module CyberpunkMP.Ink

// "Multiplayer" main menu entry: starts Night City directly through the developer world start
// (no save, prologue or character creation), then the in-world HUD connects to --ip/--port and
// the server teleports the player to a spawn point (AuthenticationResponse.spawn).
//
// Indices come from inkISystemRequestsHandler.GetWorlds()/GetGenders() on patch 2.5:
// worlds: 0 = 03_night_city, 1 = 04_main_menu; genders: 0 = Default, 1 = Female, 2 = Male.
// TODO: take the gender and appearance from the server-side character once it exists.

@wrapMethod(SingleplayerMenuGameController)
private func PopulateMenuItemList() -> Void {
    wrappedMethod();
    this.AddMenuItem("МУЛЬТИПЛЕЕР", n"CyberpunkMPStartWorld");
}

@wrapMethod(SingleplayerMenuGameController)
protected func HandleMenuItemActivate(data: ref<PauseMenuListItemData>) -> Bool {
    if Equals(data.eventName, n"CyberpunkMPStartWorld") {
        CyberpunkMPLog("[MainMenu] starting Night City without a save");
        CyberpunkMPRequestAutoConnect();
        this.GetSystemRequestsHandler().StartWorld(0, 2);
        return true;
    }
    return wrappedMethod(data);
}
