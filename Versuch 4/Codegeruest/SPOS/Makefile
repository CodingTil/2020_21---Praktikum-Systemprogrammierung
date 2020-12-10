PROJ=SPOS

# Microprocessor
MCU=atmega644

SRC:= $(wildcard $(PROJ)/*.c)

# standard linux include
INC = $(PROJ)
INC +=  /usr/avr/include

CFLAGS = \
  -x c \
  -mmcu=$(MCU) \
  -c \
  -std=c99 \
  $(addprefix -I,$(INC)) \
  -DF_CPU=20000000UL \
  -Wall \
  -funsigned-char \
  -funsigned-bitfields \
  -ffunction-sections \
  -fdata-sections \
  -fpack-struct \
  -fshort-enums \

ifneq (,$(filter $(MAKECMDGOALS),debug))
  OUT := ./bin/debug
  CFLAGS += -g3 -O0 -Wno-cpp
else
  OUT := ./bin/release
  CFLAGS += -O2
endif

LDFLAGS= \
  -Wl,--gc-sections

############

all: elf size
	@echo ""
	@echo "COMPILE SUCCESSFUL"
	@echo ""

debug: elf size
	@echo ""
	@echo "COMPILE SUCCESSFUL (DEBUG)"
	@echo ""

elf: $(PROJ).elf

OBJ=$(patsubst %.c, $(OUT)/%.o, $(SRC))

$(PROJ).elf: $(OBJ) FORCEEXEC
	avr-gcc $(LDFLAGS) -mmcu=$(MCU) $(OBJ) -o $(PROJ).elf

# Force executable creation since we place debug and release executables at the same spot:
FORCEEXEC:

$(OUT)/:
	"mkdir" -p $(OUT)
	"mkdir" -p $(OUT)/$(PROJ)

$(OUT)/%.o: %.c $(OUT)/%.d $(OUT)/
	avr-gcc -c $(CFLAGS) -MD -MP -MT $@ -MF "$(@:%.o=%.d)" -o $@ $<

print_sources:
	@$(foreach src,$(SOURCES),echo $(src);)

size:
	avr-size --mcu=$(MCU) -C $(PROJ).elf

clean:
	rm -rf ./bin
	rm -rf $(PROJ).elf


DEP=$(patsubst %.c, $(OUT)/%.d, $(SRC))
$(DEP):

include $(wildcard $(DEP))
