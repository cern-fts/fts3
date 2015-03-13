import settings
from datetime import datetime
from django.http import HttpResponse
from xml.etree.ElementTree import Element, SubElement, tostring


def _color_mapper(service_status):
    if service_status['drain'] != 0:
        return 'orange'
    elif service_status['status'] != 'running':
        return 'red'
    else:
        return 'green'


def _add_numeric(e_data, name, value):
    e_numeric = SubElement(e_data, 'numericvalue', {'name': name})
    e_numeric.text = str(value)


def _is_running(service, info):
    if service in info and info[service].get('status', '') == 'running':
        return 1
    else:
        return 0


def _calculate_availability(e_sls, servers):
    """
    Calculate the availability and its description based on the server status
    """
    e_availability_info = SubElement(e_sls, 'availabilityinfo')
    draining_count = 0
    down_count = 0
    total_count = 0
    for server, info in servers.iteritems():
        for service, service_status in info.get('services', dict()).iteritems():
            if service == 'fts_backup':
                continue
            e_subavailability = SubElement(
                e_availability_info, 'font', {
                    'style': "color:%s" % _color_mapper(service_status)
                }
            )
            e_subavailability.text = "%s %s" % (server, service)
            SubElement(e_availability_info, 'br')

            if service_status['drain'] != 0:
                draining_count += 1
                e_subavailability.tail = "Status: draining"
            elif service_status['status'] != 'running':
                down_count += 1
                e_subavailability.tail = "Status: offline"
            else:
                e_subavailability.tail = "Status: running"
            total_count += 1


    e_availability = SubElement(e_sls, 'availability')
    if total_count:
        e_availability.text = "%.2f"% float((total_count - down_count) / total_count * 100)
    else:
        e_availability.text = "0"
    if down_count > 0 or draining_count > 0:
        e_availability_desc = SubElement(e_sls, 'availabilitydesc')
        e_availability_desc.text = '%d services are down, %d draining' % (down_count, draining_count)

    e_data = SubElement(e_sls, 'data')
    _add_numeric(e_data, 'host_count', len(servers.keys()))
    _add_numeric(e_data, 'fts_server_running',
        sum(
            map(
                lambda (server, info): _is_running('fts_server', info.get('services', dict())),
                servers.iteritems()
            )
        )
    )
    _add_numeric(e_data, 'fts_bringonline_running',
        sum(
            map(
                lambda (server, info): _is_running('fts_bringonline', info.get('services', dict())),
                servers.iteritems()
            )
        )
    )


def slsfy(servers, id_tail, color_mapper=_color_mapper):
    """
    Present the data with the given filters as a suitable XML
    to be processed by SLS
    """
    e_sls = Element('serviceupdate')
    e_id = SubElement(e_sls, 'id')
    e_id.text = ("%s %s" % (getattr(settings, 'SITE_NAME', 'FTS3'), id_tail)).replace(' ', '_')

    _calculate_availability(e_sls, servers)

    e_timestamp = SubElement(e_sls, 'timestamp')
    e_timestamp.text = datetime.now().strftime('%Y-%m-%dT%H:%M:%S')

    return HttpResponse(tostring(e_sls), mimetype='application/xml')


def slsfy_error(message, id_tail):
    """
    Return an error message
    """
    e_sls = Element('serviceupdate')
    e_id = SubElement(e_sls, 'id')
    e_id.text = ("%s %s" % (getattr(settings, 'SITE_NAME', 'FTS3'), id_tail)).replace(' ', '_')

    e_availability_info = SubElement(e_sls, 'availabilityinfo')
    e_subavailability = SubElement(e_availability_info, 'font', {'style': "color:red"})
    e_subavailability.text = str(message)

    SubElement(e_sls, 'availability').text = '0'

    return HttpResponse(tostring(e_sls), mimetype='application/xml')
