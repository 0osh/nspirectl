CC := gcc
TARGET := nspirectl
SRC := main.c help.c
OBJ := $(SRC:.c=.o)
DEP := $(OBJ:.o=.d)

CFLAGS := -MMD -Wall -Wextra -Ofast
LDFLAGS := -lnspire


# % = wildcard
# $@ = target
# $< = first prerequisite
# $^ = every prerequisite

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "Linking objects -> target ($@)"
	@$(CC) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.c
	@echo "Compiling $< -> $@"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Removing dependency files (*.d)"
	@rm -f $(DEP)
	@echo "Removing object files (*.o)"
	@rm -f $(OBJ)
	@echo "Removing target ($(TARGET))"
	@rm -f $(TARGET)



-include $(DEP)
