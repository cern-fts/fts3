/*
 *  Copyright notice:
 *  Copyright © Members of the EMI Collaboration, 2010.
 *
 *  See www.eu-emi.eu for details on the copyright holders
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*  env.h

    Defines gSOAP environment shared by client and service modules

    Imports SOAP Fault and SOAP Header structures, which will be
    shared by client and service modules. The Header structure
    should contain all fields required by the clients and services.

    Copyright (C) 2000-2003 Robert A. van Engelen, Genivia inc.
    All Rights Reserved.

    Compile:
    soapcpp2 -penv env.h
    c++ -c envC.cpp
    c++ -DWITH_NONAMESPACES -c stdsoap2.cpp

*/

