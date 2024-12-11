/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sumseo <sumseo@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/07 16:00:23 by pnguyen-          #+#    #+#             */
/*   Updated: 2024/12/11 15:38:10 by sumseo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <exception>
#include <iostream>

#include "WebServer.hpp"

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
	try
	{
		WebServer	server(argv[1]);
	}
	catch (const std::exception &e)
	{
		cerr << "Error: " << e.what() << endl;
		return (1);
	}
	return (0);
}
