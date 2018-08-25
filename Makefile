# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: nfinkel <nfinkel@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2018/08/25 16:03:14 by nfinkel           #+#    #+#              #
#    Updated: 2018/08/25 16:52:15 by nfinkel          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

#################
##  VARIABLES  ##
#################

#	Environment
OS :=                   $(shell uname)

ifeq ($(HOSTTYPE),)
 HOSTTYPE :=            $(shell uname -m)_$(shell uname -s)
endif

#	Output
NAME :=	                libft_malloc_$(HOSTTYPE).so
SYMLINK :=              libft_malloc.so

#	Compiler
CC :=                   gcc
VERSION :=              -std=c11

FLAGS :=                -Wall -Wextra -Werror

ifeq ($(OS), Darwin)
	THREADS :=          $(shell sysctl -n hw.ncpu)
else
	THREADS :=          4
endif

FAST :=                 -j$(THREADS)

DYN_FLAG :=             -shared
HEADERS :=              -I ./include/
O_FLAG :=               -O2

#	Directories
OBJDIR :=               ./build/
CORE_DIR :=             ./core/
UTILS_DIR :=            ./utils/

CORE +=                 malloc.c realloc.c
UTILS +=

#	Sources
OBJECTS =               $(patsubst %.c,$(OBJDIR)%.o,$(SRCS))

SRCS +=	                $(CORE)
SRCS +=                 $(UTILS)

vpath %.c $(CORE_DIR)
vpath %.c $(UTILS_DIR)

#################
##    RULES    ##
#################

all: $(NAME)

$(NAME): $(OBJECTS)
	@$(CC) $(VERSION) $(DYN_FLAG) -o $(NAME) $(patsubst %.c,$(OBJDIR)%.o,$(notdir $(SRCS)))
	@printf  "\033[92m\033[1:32mCompiling -------------> \033[91m$(NAME)\033[0m:\033[0m%-13s\033[32m[✔]\033[0m\n"
	@ln -s $@ $(SYMLINK)

$(OBJECTS): | $(OBJDIR)

$(OBJDIR):
	@mkdir -p $@

$(OBJDIR)%.o: %.c
	@printf  "\033[1:92mCompiling $(NAME)\033[0m %-28s\033[32m[$<]\033[0m\n" ""
	@$(CC) $(VERSION) $(FLAGS) $(O_FLAG) $(HEADERS) -fpic -c $< -o $@
	@printf "\033[A\033[2K"

clean:
	@/bin/rm -rf $(OBJDIR)
	@printf  "\033[1:32mCleaning object files -> \033[91m$(NAME)\033[0m\033[1:32m:\033[0m%-13s\033[32m[✔]\033[0m\n"

fast:
	@$(MAKE) $(FAST)

fclean: clean
	@/bin/rm -f $(NAME)
	@/bin/rm -f $(SYMLINK)
	@printf  "\033[1:32mCleaning binary -------> \033[91m$(NAME)\033[0m\033[1:32m:\033[0m%-13s\033[32m[✔]\033[0m\n"

re: fclean fast

.PHONY: all clean fast fclean re
