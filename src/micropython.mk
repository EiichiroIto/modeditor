EDITOR_MOD_DIR := $(USERMOD_DIR)

# Add all C files to SRC_USERMOD.
SRC_USERMOD += $(EDITOR_MOD_DIR)/modeditor.c
SRC_USERMOD += $(EDITOR_MOD_DIR)/editor.c
SRC_USERMOD += $(EDITOR_MOD_DIR)/ucurses.c

# We can add our module folder to include paths if needed
CFLAGS_USERMOD += -I$(EDITOR_MOD_DIR)
