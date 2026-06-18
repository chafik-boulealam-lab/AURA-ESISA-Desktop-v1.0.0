// Launch the dashboard first so the application always opens visibly.
// Login remains available from the dashboard logout flow.
gboolean relaunch_login = TRUE;
while (relaunch_login) {
    gboolean dashboard_requests_login = aura_launch_dashboard(&argc, &argv);