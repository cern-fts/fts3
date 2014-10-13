import settings
from datetime import datetime
from django.http import HttpResponse
from xml.etree.ElementTree import Element, SubElement, tostring


def _color_mapper(availability):
    if availability == 100:
        return 'green'
    else:
        return 'red'


def slsfy(elements, id_tail, color_mapper=_color_mapper):
    """
    Present the data with the given filters as a suitable XML
    to be processed by SLS
    """
    e_sls = Element('serviceupdate')
    e_id = SubElement(e_sls, 'id')
    e_id.text = "%s %s" % (getattr(settings, 'SITE_NAME', 'FTS3'), id_tail)

    availability_sum = 0
    e_availability_info = SubElement(e_sls, 'availabilityinfo')
    for (name, availability) in sorted(elements, key=lambda e: e[0]):
        e_subavailability = SubElement(
            e_availability_info, 'font', {
                'style': "color:%s" % color_mapper(availability)
            }
        )
        e_subavailability.text = name.upper()
        e_subavailability.tail = "Availability: %d" % availability
        SubElement(e_availability_info, 'br')
        availability_sum += availability

    e_availability = SubElement(e_sls, 'availability')
    if len(elements):
        e_availability.text = str(availability_sum / len(elements))
    else:
        e_availability.text = '0'

    e_timestamp = SubElement(e_sls, 'timestamp')
    e_timestamp.text = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    return HttpResponse(tostring(e_sls), mimetype='application/xml')

def slsfy_error(message, id_tail):
    """
    Return an error message
    """
    e_sls = Element('serviceupdate')
    e_id = SubElement(e_sls, 'id')
    e_id.text = "%s %s" % (getattr(settings, 'SITE_NAME', 'FTS3'), id_tail)

    e_availability_info = SubElement(e_sls, 'availabilityinfo')
    e_subavailability = SubElement(e_availability_info, 'font', {'style': "color:red"})
    e_subavailability.text = str(message)

    SubElement(e_sls, 'availability').text = '0'

    return HttpResponse(tostring(e_sls), mimetype='application/xml')
