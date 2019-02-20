#################
##  VARIABLES  ##
#################

#	Environment
OS :=					$(shell uname -s)

ifeq ($(HOSTTYPE),)
	HOSTTYPE :=			$(shell uname -m)_$(OS)
endif

#	Output
NAME :=					libft_malloc_$(HOSTTYPE).so
SYMLINK :=				libft_malloc.so

#	Compiler
CC :=					gcc

#	Flags
FLAGS =					-Wall -Wextra -Wcast-align -Wconversion -Werror 
ifeq ($(OS), Darwin)
	THREADS :=			$(shell sysctl -n hw.ncpu)
else
	THREADS :=			4
endif

FAST :=					-j$(THREADS)
DYN_FLAG :=				-shared
HEADERS :=				-I ./include/
O_FLAG :=				-O3

#	Directories
OBJDIR :=				./build/
SRC_DIR :=				./src/

SRC +=					free.c malloc.c show.c

#	Sources
OBJECTS =				$(patsubst %.c,$(OBJDIR)%.o,$(SRCS))
SRCS +=					$(SRC)

vpath %.c $(SRC_DIR)

#################
##    RULES    ##
#################

all: $(NAME)

$(NAME): $(OBJECTS)
	@$(CC) $(DYN_FLAG) $(FLAGS)$(O_FLAG) $(patsubst %.c,$(OBJDIR)%.o,$(notdir $(SRCS))) -o $@
	@printf  "\033[92m\033[1;32mCompiling -------------> \033[91m$(NAME)\033[0m\033[1;32m:\033[0m%-2s\033[32m[✔]\033[0m\n"
	@ln -s $@ $(SYMLINK)

$(OBJECTS): | $(OBJDIR)

$(OBJDIR):
	@mkdir -p $@

$(OBJDIR)%.o: %.c
	@printf  "\033[1;92mCompiling $(NAME)\033[0m %-17s\033[32m[$<]\033[0m\n"
	@$(CC) $(FLAGS)$(O_FLAG) $(HEADERS) -fpic -c $< -o $@
	@printf "\033[A\033[2K"

clean:
	@/bin/rm -rf $(OBJDIR)
	@printf  "\033[1;32mCleaning object files -> \033[91m$(NAME)\033[0m\033[1;32m:\033[0m%-2s\033[32m[✔]\033[0m\n"

fast:
	@$(MAKE) --no-print-directory $(FAST)

fclean: clean
	@/bin/rm -f $(NAME)
	@/bin/rm -f $(SYMLINK)
	@printf  "\033[1;32mCleaning binary -------> \033[91m$(NAME)\033[0m\033[1;32m:\033[0m%-2s\033[32m[✔]\033[0m\n"

noflags: FLAGS := 
noflags: re

re: fclean all

purge: fclean
	@/bin/rm -rf test0
	@/bin/rm -rf test1
	@/bin/rm -rf test2
	@/bin/rm -rf test3
	@/bin/rm -rf test3_bis
	@/bin/rm -rf test4
	@/bin/rm -rf test5
	@/bin/rm -rf bonus_threading
	@/bin/rm -rf multithreading_test

.PHONY: all clean fast fclean noflags purge re
