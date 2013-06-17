from django.core.paginator import EmptyPage, PageNotAnInteger


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
