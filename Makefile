.PHONY: all clean test release desktop web

DESKTOP_DIR = apps/desktop
WEB_SERVER_DIR = apps/web/server

all: desktop

desktop:
	$(MAKE) -C $(DESKTOP_DIR) all

clean:
	$(MAKE) -C $(DESKTOP_DIR) clean

test:
	$(MAKE) -C $(DESKTOP_DIR)/tests test

web:
	$(MAKE) -C $(WEB_SERVER_DIR) all

release:
	powershell -ExecutionPolicy Bypass -File scripts/build.ps1
	powershell -ExecutionPolicy Bypass -File scripts/package_release.ps1