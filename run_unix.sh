#/bin/sh echo 'test'
export LD_LIBRARY_PATH=/home/pitch-black/Projects/ft_malloc
export LD_PRELOAD=./libft_malloc.so
export M_ABORT_ON_ERROR=0
export M_RELEASE_BIN=1
export M_SHOW_DEBUG=0
export M_SHOW_HEXDUMP=0
export M_SHOW_UNALLOCATED=1
$@
