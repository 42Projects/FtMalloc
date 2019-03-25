/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/03/25 18:27:12 by nfinkel           #+#    #+#             */
/*   Updated: 2019/03/25 18:27:29 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>

void	*calloc(size_t nmemb, size_t size);
void	free(void *ptr);
void	*malloc(size_t size);
size_t	malloc_good_size(size_t size);
size_t	malloc_size(const void *ptr);
void	*realloc(void *ptr, size_t size);
void	show_alloc_mem(void);

#endif
