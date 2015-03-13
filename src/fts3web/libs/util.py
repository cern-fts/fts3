from django.core.paginator import Paginator, PageNotAnInteger, EmptyPage
import sys
import settings


def get_order_by(request):
    order_by = request.GET.get('orderby', None)
    order_desc = False

    if not order_by:
        order_desc = True
    elif order_by[0] == '-':
        order_desc = True
        order_by = order_by[1:]

    return order_by, order_desc


def ordered_field(field, desc):
    if desc:
        return '-' + field
    return field


def get_page(paginator, request):
    try:
        if 'page' in request.GET:
            page = paginator.page(request.GET.get('page'))
        else:
            page = paginator.page(1)
    except PageNotAnInteger:
        page = paginator.page(1)
    except EmptyPage:
        page = paginator.page(paginator.num_pages)
    return page


def paged(elements, http_request, page_size=50):
    if http_request.GET.get('page', 0) == 'all':
        page_size = sys.maxint

    paginator = Paginator(elements, page_size)
    page = get_page(paginator, http_request)

    return {
        'count':      paginator.count,
        'endIndex':   page.end_index(),
        'startIndex': page.start_index(),
        'page':       page.number,
        'pageCount':  paginator.num_pages,
        'pageSize':   page_size,
        'items':      page.object_list
    }


def db_to_date():
    if settings.DATABASES['default']['ENGINE'] == 'django.db.backends.oracle':
        return 'TO_TIMESTAMP(%s, \'YYYY-MM-DD HH24:MI:SS.FF\')'
    elif settings.DATABASES['default']['ENGINE'] == 'django.db.backends.mysql':
        return 'STR_TO_DATE(%s, \'%%Y-%%m-%%d %%H:%%i:%%S\')'
    else:
        return '%s'


def db_limit(sql, limit):
    if settings.DATABASES['default']['ENGINE'] == 'django.db.backends.oracle':
        return "SELECT * FROM (%s) WHERE rownum <= %d" % (sql, limit)
    elif settings.DATABASES['default']['ENGINE'] == 'django.db.backends.mysql':
        return sql + " LIMIT %d" % limit
    else:
        return sql
