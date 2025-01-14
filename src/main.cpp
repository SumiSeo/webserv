/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sumseo <sumseo@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/07 16:00:23 by pnguyen-          #+#    #+#             */
/*   Updated: 2025/01/14 16:18:04 by pnguyen-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <exception>
#include <iostream>

#include "WebServer.hpp"

#include <iostream>

using std::cerr;
using std::cout;
using std::endl;


int	main(int argc, char **argv)
{
	char const *filename;
	if (argc != 2)
		filename = "default.conf";
	else
		filename = argv[1];
	try
	{
		WebServer	webserver(filename);
		webserver.loop();
	}
	catch (const std::exception &e)
	{
		cerr << "Error: " << e.what() << endl;
		return (1);
	}


	return (0);
}
