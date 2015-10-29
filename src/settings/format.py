# Copyright notice:
#
# Copyright (C) CERN 2013-2015
#
# Copyright (C) Members of the EMI Collaboration, 2010-2013.
#   See www.eu-emi.eu for details on the copyright holders
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# If you set this to False, Django will not format dates, numbers and
# calendars according to the current locale
USE_L10N = False

# See 
# https://docs.djangoproject.com/en/dev/ref/templates/builtins/#std:templatefilter-date
DATE_FORMAT     = 'm/d/Y'
DATETIME_FORMAT = 'd/m/Y H:i:s O'
SHORT_DATETIME_FORMAT = 'd/m/Y H:i:s'
