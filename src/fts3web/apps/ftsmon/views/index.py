from django.shortcuts import render


def index(http_request):
    return render(http_request, 'entry.html')
