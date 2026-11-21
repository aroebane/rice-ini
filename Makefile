# rice-ini - INI reader for the rice theming engines (C99, Meson).
# `make help` lists the targets.

PREFIX ?= $(HOME)/.local
BUILD_DIR ?= .build
MESON ?= meson
CLANG_FORMAT ?= clang-format
CLANG_TIDY ?= clang-tidy
BEAR ?= bear
SHELLCHECK ?= shellcheck

SOURCES := $(sort $(wildcard src/*.c include/rice-ini/*.h tests/*.c tests/*.cpp))
SCRIPTS := $(sort $(wildcard tests/*.sh scripts/*))
VERSION := $(shell cat VERSION)

.DEFAULT_GOAL := help
.PHONY: help build test check install uninstall fmt format-check lint shellcheck compdb clean release-plan release-patch release-minor release-major

help:
	@printf '%s\n' \
		'rice-ini $(VERSION)' \
		'  build         Configure and compile into $(BUILD_DIR)/' \
		'  test          Run the tests' \
		'  check         format-check, test, lint, shellcheck' \
		'  install       Install under PREFIX (default ~/.local); DESTDIR is honored' \
		'  uninstall     Remove what install put under PREFIX' \
		'  fmt           Format sources with clang-format' \
		'  format-check  Fail if a source is not formatted' \
		'  lint          Run clang-tidy over src/ (needs compile_commands.json: make compdb)' \
		'  shellcheck    Lint the test scripts' \
		'  compdb        Write compile_commands.json with bear (for clangd and clang-tidy)' \
		'  release-plan  Show the next patch release without changing anything' \
		'  release-patch|minor|major  Check, bump VERSION, sign the tag, push (see scripts/release)' \
		'  clean         Remove $(BUILD_DIR)/ and compile_commands.json' \
		'Variables: PREFIX BUILD_DIR DESTDIR'

$(BUILD_DIR)/build.ninja:
	$(MESON) setup "$(BUILD_DIR)" --prefix "$(PREFIX)"

build: $(BUILD_DIR)/build.ninja
	$(MESON) compile -C "$(BUILD_DIR)"

test: build
	$(MESON) test -C "$(BUILD_DIR)" --print-errorlogs

install: build
	$(MESON) configure "$(BUILD_DIR)" --prefix "$(PREFIX)"
	$(MESON) install -C "$(BUILD_DIR)"

uninstall: $(BUILD_DIR)/build.ninja
	ninja -C "$(BUILD_DIR)" uninstall

fmt:
	$(CLANG_FORMAT) -i $(SOURCES)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(SOURCES)

compdb: $(BUILD_DIR)/build.ninja
	ninja -C "$(BUILD_DIR)" -t clean >/dev/null
	$(BEAR) --output compile_commands.json -- $(MESON) compile -C "$(BUILD_DIR)"

lint: compdb
	$(CLANG_TIDY) --config-file=.clang-tidy -p . src/*.c

shellcheck:
	$(SHELLCHECK) $(SCRIPTS)

check: format-check test lint shellcheck

# Cuts the next release; see scripts/release --help. Needs the release key: export GNUPGHOME first.
release-plan:
	@bash scripts/release patch --plan $(RELEASE_ARGS)

release-patch release-minor release-major: release-%:
	@bash scripts/release $* $(RELEASE_ARGS)

clean:
	rm -rf -- "$(BUILD_DIR)" compile_commands.json
