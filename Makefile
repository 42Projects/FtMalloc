# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: nfinkel <nfinkel@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2018/08/25 16:03:14 by nfinkel           #+#    #+#              #
#    Updated: 2019/01/29 16:20:39 by nfinkel          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

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
VERSION :=				-std=c11

#	Flags
ifeq ($(OS), Darwin)
	FLAGS +=			-Wall -Wextra -Werror 
	THREADS :=			$(shell sysctl -n hw.ncpu)
else
	THREADS :=			4
endif

FAST :=					-j$(THREADS)
DYN_FLAG :=				-shared
HEADERS :=				-I ./include/
O_FLAG :=				-O2

#	Directories
LIBFTDIR :=				./libft/
OBJDIR :=				./build/
SRC_DIR :=				./src/

SRC +=					free.c malloc.c realloc.c

#	Sources
OBJECTS =				$(patsubst %.c,$(OBJDIR)%.o,$(SRCS))
SRCS +=					$(SRC)

vpath %.c $(SRC_DIR)

#################
##    RULES    ##
#################

all: $(NAME)

#	@$(CC) $(VERSION) $(DYN_FLAG)$(FLAGS) $(O_FLAG) $(patsubst %.c,$(OBJDIR)%.o,$(notdir $(SRCS))) -L $(LIBFTDIR) -lft -o $@
$(NAME): $(OBJECTS)
	@$(CC) $(VERSION) $(DYN_FLAG)$(FLAGS) $(O_FLAG) $(patsubst %.c,$(OBJDIR)%.o,$(notdir $(SRCS))) -o $@
	@printf  "\033[92m\033[1;32mCompiling -------------> \033[91m$(NAME)\033[0m:\033[0m%-13s\033[32m[✔]\033[0m\n"
	@ln -s $@ $(SYMLINK)

$(OBJECTS): | $(OBJDIR)

$(OBJDIR):
	@mkdir -p $@

$(OBJDIR)%.o: %.c
	@printf  "\033[1;92mCompiling $(NAME)\033[0m %-28s\033[32m[$<]\033[0m\n"
	@$(CC) $(VERSION) $(FLAGS) $(O_FLAG) $(HEADERS) -fpic -c $< -o $@
	@printf "\033[A\033[2K"

clean:
	@/bin/rm -rf $(OBJDIR)
	@printf  "\033[1;32mCleaning object files -> \033[91m$(NAME)\033[0m\033[1;32m:\033[0m%-13s\033[32m[✔]\033[0m\n"

fast:
	@$(MAKE) --no-print-directory $(FAST)

fclean: clean
	@/bin/rm -f $(NAME)
	@/bin/rm -f $(SYMLINK)
	@printf  "\033[1;32mCleaning binary -------> \033[91m$(NAME)\033[0m\033[1;32m:\033[0m%-13s\033[32m[✔]\033[0m\n"

libft:
	@$(MAKE) -C $(LIBFTDIR)

noflags: FLAGS := 
noflags: re

purge: fclean
	@$(MAKE) fclean -C $(LIBFTDIR)

re: fclean all

.PHONY: all clean fast fclean libft noflags purge re
