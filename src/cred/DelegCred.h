/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DELEGCRED_H_
#define DELEGCRED_H_

/**
 * DelegCred API.
 * Define the interface for retrieving the User Credentials for a given user DN
 */
class DelegCred
{
public:

    /**
     * Get name of a file containing the credentials for the requested user
     * @param userDn [IN] The distinguished name of the user
     * @param id [IN] The credential id needed to retrieve the user's
     *        credentials (may be a password or an identifier, depending on the
     *        implementation)
     */
    static std::string getProxyFile(const std::string &userDn, const std::string &id);

    /**
     * Returns true if the certificate in the given file name is still valid
     * @param filename [IN] the name of the file containing the proxy certificate
     * @return true if the certificate in the given file name is still valid
     */
    static bool isValidProxy(const std::string &filename, std::string &message);

    /**
     * Generate a name for the file that should contain the proxy certificate.
     * The length of this name should be (MAX_FILENAME - 7) maximum.
     * @param userDn [IN] the user DN passed to the get method
     * @param id [IN] the credential id passed to the get method
     * @param legacy [IN] when true, generate the proxy file name by encoding the user DN,
     *                    otherwise append the credential ID
     * @return the generated file name
     */
    static std::string generateProxyName(const std::string &userDn, const std::string &id, bool legacy = false);

private:
    /**
     * Get a new Certificate and store in into a file
     * @param userDn [IN] the user DN passed to the get method
     * @param id [IN] the credential id passed to the get method
     * @param fname [IN] the name of a temporary file where the new proxy
     * certificate should be stored
     */
    static void getNewCertificate(const std::string &userDn, const std::string &credId, const std::string &fname);

    /**
     * Returns the validity time that the cached copy of the certificate should
     * have.
     * @return the validity time that the cached copy of the certificate should
     * have
     */
    static unsigned long minValidityTime();

    /**
     * Forbid instantiation
     */
    DelegCred();
};


#endif // DELEGCRED_H_

