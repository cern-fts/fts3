
function pairState(pair)
{
	var klasses;

	// No active with submitted is bad, and so it is
	// less than three active and more than three submitted
	if ((!pair.active && pair.submitted) || (pair.active < 2 && pair.submitted >= 2))
		klasses = 'bad-state';
	// Very high rate of failures, that's pretty bad
	else if ((!pair.finished && pair.failed) || (pair.finished / pair.failed <= 0.8))
		klasses = 'bad-state';
	// Less than three actives is so-so
	else if (pair.active < 2)
		klasses = 'underused';
	// More than three active, that's good enough
	else if (pair.active >= 2)
		klasses = 'good-state';
	// High rate of success, that's good
	else if ((pair.finished && !pair.failed) || (pair.finished / pair.failed >= 0.9))
		klasses = 'good-state';
	// Meh
	else
		klasses = '';
	
	// If any active, always give that
	if (pair.active)
		klasses += ' active';	
	
	return klasses;
}

function mergeAttrs(a, b)
{
	for (var attr in b) {
		if (typeof(b[attr]) == 'string')
			a[attr] = b[attr];
		else
			a[attr] = '';
	}
	return a;
}

function OverviewCtrl($location, $scope, overview, Overview)
{
	$scope.overview = overview;

	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};
	
	// Method to choose a style for a pair
	$scope.pairState = pairState;
	
	// Filter
	$scope.filterBy = function(filter) {
		$location.search(filter);
	}

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.overview.page;
        Overview.query(filter, function(updatedOverview) {
            for(var i = 0; i < updatedOverview.items.length; i++) {
                updatedOverview.items[i].show = $scope.overview.items[i].show;
            }
            scope.overview = updatedOverview;
        });
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


OverviewCtrl.resolve = {
	overview: function($rootScope, $location, $q, Overview) {
		loading($rootScope);
		
		var deferred = $q.defer();
	
		var page = $location.search().page;
		if (!page || page < 1)
			page = 1;
		
		Overview.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));
		
		return deferred.promise;
	}
}
