/*
 * File Name:   traceutil.h
 * Description: debug tracing
 *
 * Copyright (C) 2023 Dieter J Kybelksties <github@kybelksties.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * @date: 2023-08-28
 * @author: Dieter J Kybelksties
 */

#ifndef NS_UTIL_TRACEUTIL_H_INCLUDED
#define NS_UTIL_TRACEUTIL_H_INCLUDED

#include <iostream>

#if defined DO_TRACE_

    #define TRACE0                                                                        \
        {                                                                                 \
            std::cout << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << std::endl; \
        }
    #define TRACE1(v1)                                                                                               \
        {                                                                                                            \
            std::cout << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " " << #v1 << "=" << v1 << std::endl; \
        }
    #define TRACE2(v1, v2)                                                                                      \
        {                                                                                                       \
            std::cout << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " "                              \
                      << #v1 << "=" << v1                                                                       \
                      << " "                                                                                    \
                      << #v2 << "=" << v2                                                                       \
                      << std::endl;                                                                             \
        }
    #define TRACE3(v1, v2, v3)                                                                                  \
        {                                                                                                       \
            std::cout << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " "                              \
                      << #v1 << "=" << v1                                                                       \
                      << " "                                                                                    \
                      << #v2 << "=" << v2                                                                       \
                      << " "                                                                                    \
                      << #v3 << "=" << v3                                                                       \
                      << std::endl;                                                                             \
        }
    // multi process versions for forked processes
    #define PTRACE0                                                                                              \
        {                                                                                                        \
            std::cout << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " PID=" << getpid() << std::endl; \
        }
    #define PTRACE1(v1)                                                                                          \
        {                                                                                                        \
            std::cout << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " PID=" << getpid() << " " << #v1 \
                      << "=" << v1 << std::endl;                                                                 \
        }
    #define PTRACE2(v1, v2)                                                                                     \
        {                                                                                                       \
            std::cout << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " PID=" << getpid() << " "       \
                      << #v1 << "=" << v1                                                                       \
                      << " "                                                                                    \
                      << #v2 << "=" << v2                                                                       \
                      << std::endl;                                                                             \
        }
    #define PTRACE3(v1, v2, v3)                                                                                 \
        {                                                                                                       \
            std::cout << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " PID=" << getpid() << " "       \
                      << #v1 << "=" << v1                                                                       \
                      << " "                                                                                    \
                      << #v2 << "=" << v2                                                                       \
                      << " "                                                                                    \
                      << #v3 << "=" << v3                                                                       \
                      << std::endl;                                                                             \
        }

#else

    #define TRACE0
    #define TRACE1(v1)
    #define TRACE2(v1, v2)
    #define TRACE3(v1, v2, v3)
    // multi process versions for forked processes
    #define PTRACE0
    #define PTRACE1(v1)
    #define PTRACE2(v1, v2)
    #define PTRACE3(v1, v2, v3)

#endif  // DO_TRACE_

#endif  // NS_UTIL_TRACEUTIL_H_INCLUDED
