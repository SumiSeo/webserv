/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sumseo <sumseo@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/07 16:00:23 by pnguyen-          #+#    #+#             */
/*   Updated: 2024/12/03 12:23:01 by sumseo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>

using std::cerr;
using std::cout;
using std::endl;

int	main(int argc, char **argv)
{
	if (argc != 2)
	{
		cerr << "Usage: " << argv[0] << " <configuration file>" << endl;
		return (1);
	}
	cout << "Initial Release" << endl;
	return (0);
}
