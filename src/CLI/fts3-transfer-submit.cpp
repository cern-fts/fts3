#include "ftsFileTransferSoapBindingProxy.h"
#include "fts.nsmap"
#include "evn.h"
#include "SubmitTransferCli.h"
#include "CliManager.h"

#include "ServiceDiscoveryIfce.h"
#include <glite/data/glite-util.h>

#include <cgsi_plugin.h>
#include <gridsite.h>


#include <openssl/x509_vfy.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>

#include <time.h>

using namespace std;

long isCertValid ();

int main(int ac, char* av[]) {

	// create service
	FileTransferSoapBindingProxy service;

	try {
    	SubmitTransferCli cli;
    	cli.initCli(ac, av);

    	string source = cli.getSource(), destination = cli.getDestination(), endpoint = cli.getService();
		// set endpoint
		if (endpoint.size() == 0) {
			cout << "Failed to determine the endpoint" << endl;
			return 0;
		}

		service.soap_endpoint = endpoint.c_str();	// https://vtb-generic-32.cern.ch:8443/glite-data-transfer-fts/services/FileTransfer

		// init
		soap_cgsi_init(&service,  CGSI_OPT_DISABLE_NAME_CHECK | CGSI_OPT_SSL_COMPATIBLE);
		soap_set_namespaces(&service, fts_namespaces);

		fts__getInterfaceVersionResponse ivresp;
		service.getInterfaceVersion(ivresp);
		string interface = ivresp.getInterfaceVersionReturn;

		CliManager::getInstance()->setInterface(interface);

    	if(!cli.performChecks()) return 0;

		if (cli.printHelp() || cli.printVersion()) return 0;

		// produce output
		if (cli.isVerbose()) {
			cout << "# Using endpoint" << service.soap_endpoint << endl;

			fts__getVersionResponse version;
			service.getVersion(version);
			cout << "# Service version: " << version.getVersionReturn << endl;

			fts__getInterfaceVersionResponse interface;
			service.getInterfaceVersion(interface);
			cout << "# Interface version: " << interface.getInterfaceVersionReturn << endl;

			fts__getSchemaVersionResponse schema;
			service.getSchemaVersion(schema);
			cout << "# Schema version: " << schema.getSchemaVersionReturn << endl;

			fts__getServiceMetadataResponse metadata;
			service.getServiceMetadata("feature.string", metadata);
			cout << "# Service features: " << metadata._getServiceMetadataReturn << endl;
		}
		// Job
		transfer__TransferJob job;

		// JobElement
		transfer__TransferJobElement element;
		element.source = soap_new_std__string(&service, -1);
		element.dest = soap_new_std__string(&service, -1);

		*(element.source) = source;//"srm://lxbra1910.cern.ch:8446/srm/managerv2?SFN=/dpm/cern.ch/home/dteam/file.out";
		*(element.dest) = destination;//"srm://galway.desy.de:8443/srm/managerv2?SFN=/pnfs/desy.de/data/dteam/file.out.overwrite";

		job.transferJobElements.push_back(&element);

		// Params
		job.jobParams = cli.getParams(&service);
/*
		job.jobParams = soap_new_transfer__TransferParams(&service, -1);
		job.jobParams->keys.push_back("overwrite");
		job.jobParams->values.push_back("Y");
*/
		// transfer submit
		fts__transferSubmit2Response resp;
		int ret = service.transferSubmit2(&job, resp);

		// resp
		cout << "ret: " << ret << endl;
		cout << "resp: " << resp._transferSubmit2Return << endl;
    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        cerr << "Exception of unknown type!\n";
    }

    return 0;
}

long isCertValid () {
	string serviceLocation = "https://vtb-generic-32.cern.ch:8443/glite-data-transfer-fts/services/gridsite-delegation";

    char * user_proxy = GRSTx509FindProxyFileName();
	FILE *fp = fopen(user_proxy , "r");
    X509 *cert = PEM_read_X509(fp, 0, 0, 0);
    fclose(fp);
    char* c_str = (char*) ASN1_STRING_data(X509_get_notAfter(cert));
    long time = GRSTasn1TimeToTimeT(c_str, 0) - ::time(0);

    cout << "Remaining time for local proxy is " << time / 3600 << " hours and " << time % 3600 / 60 << " minutes." << endl;

    return time;
}
