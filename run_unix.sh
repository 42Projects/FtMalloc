#/bin/sh echo 'test'
export LD_LIBRARY_PATH=/home/pitch-black/Projects/ft_malloc
export LD_PRELOAD=./libft_malloc.so
$@
