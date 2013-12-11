import config
import copy
from urlparse import urlparse


def getStoragePairs():
    """
    From the configured areas, generates a list of pairs
    """
    pairs = []
    for (src, dst) in config.StorageAreaPairs:
        src = src % config.StorageParametrization
        dst = dst % config.StorageParametrization
        pairs.append((src, dst))
    return pairs


def getStorageElement(surl):
    """
    Return only the relevant part for the configuration from the surl
    """
    parsed = urlparse(surl)
    return "%s://%s" % (parsed.scheme, parsed.hostname)
