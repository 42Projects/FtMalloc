/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nfinkel <nfinkel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2018/08/25 16:43:22 by nfinkel           #+#    #+#             */
/*   Updated: 2018/08/25 20:31:06 by nfinkel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <malloc.h>

void	*ft_malloc(t_malloc g_malloc, size_t size) {
	if (g_malloc == NULL) {
		;
	}

	return (NULL);
}

void	*malloc(size_t size)
{
	return ft_malloc(NULL, size);
}
