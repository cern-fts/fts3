from django.shortcuts import render

def index(httpRequest):
    return render(httpRequest, 'entry.html')
