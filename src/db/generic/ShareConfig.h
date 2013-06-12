/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 *
 * ShareConfig.h
 *
 *  Created on: Nov 21, 2012
 *      Author: simonm
 */

#ifndef SHARECONFIG_H_
#define SHARECONFIG_H_


class ShareConfig
{
public:
    ShareConfig(): active_transfers(0), share_only(false) {};
    ~ShareConfig() {};

    std::string source;
    std::string destination;
    std::string vo;
    int active_transfers;
    bool share_only;
};

#endif /* SHARECONFIG_H_ */
