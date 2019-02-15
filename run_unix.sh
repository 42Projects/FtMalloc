#/bin/sh echo 'test'
export LD_PRELOAD=./libft_malloc.so
$@
