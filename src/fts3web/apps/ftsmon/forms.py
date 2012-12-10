from django import forms

class JobSearchForm(forms.Form):
    jobId = forms.CharField(required = False)
