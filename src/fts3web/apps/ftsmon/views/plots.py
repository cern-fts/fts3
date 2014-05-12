# Copyright notice:
# Copyright (C) Members of the EMI Collaboration, 2010.
#
# See www.eu-emi.eu for details on the copyright holders
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

import colorsys
import os
import tempfile
os.environ['MPLCONFIGDIR'] = tempfile.mkdtemp()

from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
from matplotlib.figure import Figure
from matplotlib.font_manager import FontProperties
from matplotlib.ticker import MaxNLocator
from django.http import HttpResponse


def _strToArray(str):
    a = []
    for e in str.split(','):
        if e:
            a.append(e)
    return a


def _error(httpRequest, msg = 'Error on plotting. Probably wrong query format.'):
    fig = Figure(figsize = (3, 3))
    canvas = FigureCanvas(fig)
    
    ax = fig.add_subplot(111)
    ax.text(0.5 , 0.5, msg,
            horizontalalignment='center',
            verticalalignment='center',
            transform = ax.transAxes)
    ax.set_axis_off()
    
    response = HttpResponse(content_type = 'image/png')
    fig.savefig(response, format='png')
    return response


def _tetrad(h, s, v):
    t = [((0.25 * x + h), s, v) for x in range(4)]
    return t


def _generateColors(number):
    if number <= 0:
        return []
    
    # Starting hue
    hStart = 0.61
    
    # Append tetrads displacing the start depending on how many
    # elements we have
    dispNumber = number / 4
    if number % 4 > 0:
        dispNumber = dispNumber + 1
    dispRange  = 0.25
    dispStep   = dispRange / float(dispNumber)
    HSV_tuples = []
    for i in range(dispNumber):
        h = hStart + dispStep * i
        HSV_tuples.extend(_tetrad(h, 0.75, 0.85))
    
    RGB_tuples = map(lambda hsv: colorsys.hsv_to_rgb(*hsv), HSV_tuples)
    RGB_255_tuples = map(lambda rgb: (rgb[0] * 255, rgb[1] * 255, rgb[2] * 255), RGB_tuples)
    WEB_tuples = map(lambda rgb: "%02X%02X%02X" % rgb, RGB_255_tuples)
    
    return WEB_tuples[:number]


def _adjust(values, length, default = 0):
    if values is not None:
        missing = length - len(values)
        if missing > 0:
            values.extend([default] * missing)
        elif missing < 0:
            values = values[:length]

# Draw a pie
def pie(httpRequest):
    try:
        labels = []
        values = []
        colors = None
        title  = None
        suffix = ''

        for (arg, argv) in httpRequest.GET.iteritems():
            if arg == 't':
                title = argv
            elif arg == 'l':
               labels = _strToArray(argv)
            elif arg == 'v':
                values = _strToArray(argv)
            elif arg == 'c':
                cx = _strToArray(argv)
                if len(cx):
                    colors = cx
            elif arg == 'suffix':
                suffix = ' ' + argv

        n_items = len(labels)
        _adjust(values, n_items)

        # Generate color list if not specified
        if not colors:
            colors = _generateColors(n_items)
        else:
            _adjust(colors, n_items, '0')

        # Append # if the color is a full RGB                    
        for c in range(n_items):
            if len(colors[c]) > 1:
                colors[c] = '#' + colors[c]

        if not values:
            return _error(httpRequest, 'No values')

        values = map(float, values)

        if sum(values) == 0:
            return _error(httpRequest, 'Total is 0')

        # If lc is specified, put the values into the legend
        if httpRequest.GET.get('lc', False):
            for li in range(n_items):
                labels[li] = "%s (%.2f%s)" % (labels[li], values[li], suffix)

        fig = Figure(figsize = (6,3))
        canvas = FigureCanvas(fig)

        ax = fig.add_subplot(1,2,1)
        (patches, texts, auto) = ax.pie(values, labels=None, colors=colors, autopct='%1.1f%%')
        if title:
            ax.set_title(title)

        for p in patches:
            p.set_edgecolor('white')

        ax2 = fig.add_subplot(1,2,2)
        fontP = FontProperties()
        fontP.set_size('small')
        ax2.legend(patches, labels, loc='center left', prop = fontP)
        ax2.set_axis_off()

        response = HttpResponse(content_type = 'image/png')
        fig.savefig(response, format='png', bbox_inches = 'tight', transparent = True)

        return response
    except Exception, e:
        return _error(httpRequest, str(e))


def _safe_float(f):
    try:
        return float(f)
    except:
        return 0.0

# Draw a line chart
def lines(httpRequest):
    try:
        left = None
        right = None
        x_axis = None
        title = None
        label_left = None
        label_right = None

        for (arg, argv) in httpRequest.GET.iteritems():
            if arg == 't':
                title = argv
            elif arg == 'l':
               left =  map(lambda x: _safe_float(x), _strToArray(argv))
            elif arg == 'r':
                right = map(lambda x: _safe_float(x), _strToArray(argv))
            elif arg == 'x':
                x_axis = _strToArray(argv)
            elif arg == 'll':
                label_left = argv
            elif arg == 'lr':
                label_right = argv
                
        n_items = len(x_axis)

        if not n_items:
            raise Exception("No data available")

        # Fill or truncate
        _adjust(left, n_items)
        _adjust(right, n_items)

        # Draw
        x_range = range(len(x_axis))
        (left_color, right_color) = ('#0044cc', '#51a351')
        
        fig = Figure(figsize = (15,3))
        canvas = FigureCanvas(fig)
        
        ax_left = fig.add_subplot(1,1,1)
        ax_left.plot(x_range, left, color=left_color, lw=2)
        ax_left.set_ylim(0, max(left) + 1)
        ax_left.grid()
        if label_left:
            ax_left.set_ylabel(label_left, color=left_color)
        if title:
            ax_left.set_title(title)
        
        if right:
            ax_right = ax_left.twinx()
            ax_right.plot(x_range, right, color=right_color, lw=2)
            ax_right.set_ylim(0)
            if label_right:
                ax_right.set_ylabel(label_right, color=right_color)

        ax_left.get_yaxis().set_major_locator(MaxNLocator(integer=True))
        ax_left.set_xticks(x_range)
        x_labels = ax_left.set_xticklabels(x_axis)
        for l in x_labels:
            l.set_horizontalalignment('right')
            l.set_rotation(30)
            l.set_size('small')
        
        response = HttpResponse(content_type = 'image/png')
        fig.savefig(response, format='png', bbox_inches = 'tight', transparent = True)
        
        return response
    except Exception, e:
        return _error(httpRequest, str(e))
