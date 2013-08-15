import config
from urlparse import urlparse



def groupStorageByProtocol(areas):
	"""
	From the configured areas, generates a dictionary where the urls are
	grouped by protocol
	"""
	byProtocol = {}
	for area in areas:
		url = urlparse(area)
		if url.scheme not in byProtocol:
			byProtocol[url.scheme] = []
		byProtocol[url.scheme].append(area % config.StorageParametrization)
	return byProtocol



def getStoragePairs():
	"""
	From the configured areas, generates a list of pairs with the same
	protocol.
	"""
	byProtocol = groupStorageByProtocol(config.StorageAreas)
	pairs = []
	for (protocol, areas) in byProtocol.iteritems():
		for source in areas:
			for destination in [a for a in areas if a != source]:
				pairs.append((source, destination))

	return pairs

