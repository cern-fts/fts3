angular.module('ftsmon.plots', [])
.directive('plotPie', function() {
    return {
        restrict: 'E',
        scope: 'isolate',
        template: '<img ng-src="plot/pie?t={{title}}&l={{labels}}&v={{values}}&c={{colors}}&lc={{legend_count}}&suffix={{suffix}}"></img>',
        link: function(scope, iterStartElement, attr) {
            var list = scope.$eval(attr.list);
            // If label and value are specified, list is an array of
            // objects, and label and value specify the fields to use
            if (attr.label && attr.value)
                plotPieArrayOfObjects(scope, list, attr.label, attr.value);
            // If labels is specified, then list is an object, and labels
            // specify the attributes to plot
            else if (attr.labels)
                plotPieObject(scope, list, attr.labels.split(','));
            // If only value is specified, then the list is a dictionary,
            // value specify the field to plot for each entry
            else if (attr.value)
                plotPieDictionary(scope, list, attr.value);
            // Otherwise, assume a dictionary of key=value
            else
                plotPieKeyValue(scope, list);

            // Show the numbers in the legend?
            if (attr.legendCount)
                scope.legend_count = 1;
            if (attr.suffix)
                scope.suffix = attr.suffix;

            // Set title and colors
            scope.title  = attr.title;
            if (attr.colors)
                scope.colors = attr.colors;
        }
    };
})
.directive('plotLines', function() {
    return {
        restrict: 'E',
        scope: 'isolate',
        template: '<img ng-src="plot/lines?t={{title}}&ll={{label_left}}&l={{values_left}}&lr={{label_right}}&r={{values_right}}&x={{x_axis}}"></img>',
        link: function(scope, iterStartElement, attr) {
            var list = scope.$eval(attr.list);

            scope.title = attr.title;
            scope.label_left = firstUpper(attr.yLeft);
            scope.label_right = firstUpper(attr.yRight);

            scope.x_axis = '';
            scope.values_left = '';
            scope.values_right = '';
            var i = list.length;
            while (i--) {
            	scope.x_axis += list[i][attr.x] + ',';
            	scope.values_left += list[i][attr.yLeft] + ',';
            	scope.values_right += list[i][attr.yRight] + ',';
            }
        }
    };
})

/** Pie plotting */
function plotPieArrayOfObjects(scope, list, labelAttr, valueAttr)
{
    var labelStr = '', valueStr = '';
    for (var i in list) {
        var item = list[i];
        labelStr += firstUpper(item[labelAttr]) + ',';
        valueStr += undefinedAsZero(item[valueAttr]) + ',';
    }

    scope.labels = labelStr;
    scope.values = valueStr;
}

function plotPieObject(scope, obj, labelsAttr)
{
    var labelStr = '', valueStr = '';
    for (var i in labelsAttr) {
        var label = labelsAttr[i];
        labelStr += firstUpper(label) + ',';
        valueStr += undefinedAsZero(obj[label]) + ',';
    }

    scope.labels = labelStr;
    scope.values = valueStr;
}

function plotPieDictionary(scope, list, value)
{
    var labelStr = '', valueStr = '';
    for (var i in list) {
        if (i[0] != '$') {
            labelStr += i + ',';
            valueStr += undefinedAsZero(list[i][value]) + ',';
        }
    }

    scope.labels = labelStr;
    scope.values = valueStr;
}

function plotPieKeyValue(scope, list)
{
    var labelStr = '', valueStr = '';
    for (var key in list) {
        if (key[0] != '$') {
            labelStr += key + ',';
            valueStr += undefinedAsZero(list[key]) + ',';
        }
    }

    scope.labels = labelStr;
    scope.values = valueStr;
}