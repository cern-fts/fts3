from django.shortcuts import render
from authn import require_certificate

@require_certificate
def index(http_request):
    return render(http_request, 'entry.html')
