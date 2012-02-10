/*
 * CliManager.cpp
 *
 *  Created on: Feb 7, 2012
 *      Author: simonm
 */

#include "CliManager.h"

#include <termios.h>
#include <iostream>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
using namespace boost;

CliManager::CliManager() {

	FTS3_PARAM_GRIDFTP = "gridftp";
	FTS3_PARAM_MYPROXY = "myproxy";
	FTS3_PARAM_DELEGATIONID = "delegationid";
	FTS3_PARAM_SPACETOKEN = "spacetoken";
	FTS3_PARAM_SPACETOKEN_SOURCE = "source_spacetoken";
	FTS3_PARAM_COPY_PIN_LIFETIME = "copy_pin_lifetime";
	FTS3_PARAM_LAN_CONNECTION = "lan_connection";
	FTS3_PARAM_FAIL_NEARLINE = "fail_nearline";
	FTS3_PARAM_OVERWRITEFLAG = "overwrite";
	FTS3_PARAM_CHECKSUM_METHOD = "checksum_method";

	FTS3_CA_SD_TYPE = "org.glite.ChannelAgent";
	FTS3_SD_ENV = "GLITE_SD_FTS_TYPE";
	FTS3_SD_TYPE = "org.glite.FileTransfer";
	FTS3_IFC_VERSION = "GLITE_FTS_IFC_VERSION";
	FTS3_INTERFACE_VERSION = "3.7.0"; // TODO where it comes from?
}

CliManager::CliManager(CliManager const&) {

}

CliManager& CliManager::operator=(CliManager const&) {
	return *this;
}

CliManager::~CliManager() {
	delete manager;
}

CliManager* CliManager::manager = 0;

CliManager* CliManager::getInstance() {

	if (!manager) {
		manager = new CliManager();
	}
	return CliManager::manager;
}

void CliManager::setInterface(string interface) {

	if (interface.empty()) return;


	char_separator<char> sep(".");
	tokenizer< char_separator<char> > tokens(interface, sep);
	tokenizer< char_separator<char> >::iterator it = tokens.begin();

	if (it == tokens.end()) return;

	string s = *it++;
	major = lexical_cast<long>(s);

	if (it == tokens.end()) return;

	s = *it++;
	minor = lexical_cast<long>(s);

	if (it == tokens.end()) return;

	s = *it;
	patch = lexical_cast<long>(s);
}

string CliManager::getPassword() {

    termios stdt;
    // get standard cmd settings
    tcgetattr(STDIN_FILENO, &stdt);
    termios newt = stdt;
    // turn off echo while typing
    newt.c_lflag &= ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt)) {
    	cout << "submit: could not set terminal attributes" << endl;
    	tcsetattr(STDIN_FILENO, TCSANOW, &stdt);
    	return "";
    }

    string pass1, pass2;

    cout << "Enter MyProxy password: ";
    cin >> pass1;
    cout << endl << "Enter MyProxy password again: ";
    cin >> pass2;
    cout << endl;

    tcsetattr(STDIN_FILENO, TCSANOW, &stdt);

    if (pass1.compare(pass2)) {
    	cout << "Entered MyProxy passwords do not match." << endl;
    	return "";
    }

    return pass1;
}

int CliManager::isChecksumSupported() {
    return isItVersion370();
}

int CliManager::isDelegationSupported() {
    return isItVersion330();
}

int CliManager::isRolesOfSupported() {
    return isItVersion330();
}

int CliManager::isSetTCPBufferSupported() {
    return isItVersion330();
}

int CliManager::isExtendedChannelListSupported() {
    return isItVersion330();
}

int CliManager::isChannelMessageSupported() {
    return isItVersion330();
}

int CliManager::isUserVoRestrictListingSupported() {
    return isItVersion330();
}

int CliManager::isVersion330StatesSupported(){
    return isItVersion330();
}

// TODO !!!

int CliManager::isItVersion330() {
    return major >= 3 && minor >= 3 && patch >= 0;
}

int CliManager::isItVersion340() {
    return major >= 3 && minor >= 4 && patch >= 0;
}

int CliManager::isItVersion350() {
    return major >= 3 && minor >= 5 && patch >= 0;
}

int CliManager::isItVersion370() {
	return major >= 3 && minor >= 7 && patch >= 0;
}

