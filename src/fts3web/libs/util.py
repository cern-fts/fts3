
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
