PROJ=Versuch_0

# Microprocessor
MCU=atmega644

SRC:= $(wildcard $(PROJ)/*.c)

OUT = ./bin

# standard linux include
INC = $(PROJ)
INC +=  /usr/avr/include
  
CFLAGS = \
  -x c \
  -O2 \
  -g2 \
  -mmcu=$(MCU) \
  -c \
  -std=gnu99 \
  $(addprefix -I,$(INC)) \
  -DF_CPU=20000000UL \
  -Wall \
  -Werror \
  -funsigned-char \
  -funsigned-bitfields \
  -ffunction-sections \
  -fdata-sections \
  -fpack-struct \
  -fshort-enums \

LDFLAGS= \
  -Wl,--gc-sections

############ 

all: elf size
	@echo ""
	@echo "COMPILE SUCCESSFUL"
	@echo ""

elf: $(PROJ).elf

OBJ=$(patsubst %.c, $(OUT)/%.o, $(SRC))

$(PROJ).elf: $(OBJ)
	avr-gcc $(LDFLAGS) -mmcu=$(MCU) $(OBJ) -o $(PROJ).elf

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
	rm -rf $(OUT)
	rm -rf $(PROJ).elf


DEP=$(patsubst %.c, $(OUT)/%.d, $(SRC))
$(DEP):

include $(wildcard $(DEP))
