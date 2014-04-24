from django.core.paginator import Paginator, PageNotAnInteger, EmptyPage


def getOrderBy(request):
    orderBy = request.GET.get('orderby', None)
    orderDesc = False
    
    if not orderBy:
        orderDesc = True
    elif orderBy[0] == '-':
        orderDesc = True
        orderBy = orderBy[1:]
    
    return (orderBy, orderDesc)


def orderedField(field, desc):
    if desc:
        return '-' + field
    return field


def getPage(paginator, request):
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


def paged(elements, httpRequest, pageSize=50):    
    if httpRequest.GET.get('page', 0) == 'all':
        pageSize = sys.maxint
    
    paginator = Paginator(elements, pageSize)
    page = getPage(paginator, httpRequest)
    
    paged = {
        'count':      paginator.count,
        'endIndex':   page.end_index(),
        'startIndex': page.start_index(),
        'page':       page.number,
        'pageCount':  paginator.num_pages,
        'pageSize':   pageSize,
        'items':      page.object_list
    }
    
    return paged
