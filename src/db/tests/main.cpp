#include "process.h"
#include <iostream>

int main(){

	const std::string cmd = "fts3_url_copy";
	std::string params = " --source_url ";
	params.append("source url");
	params.append("--dest_url ");
	params.append("dest url");
	Process *pr = new Process(cmd, params, 0);
	pr->executeProcessShell();

	return (0);
} 
