
function StatsOverviewCtrl($location, $scope, stats, Job)
{
	$scope.stats = stats;
}

StatsOverviewCtrl.resolve = {
		stats: function($rootScope, $location, $q, Statistics) {
	    	loading($rootScope);
	    	
	    	var deferred = $q.defer();

	    	var page = $location.search().page;
	    	if (!page || page < 1)
	    		page = 1;
	    	
	    	Statistics.query($location.search(), function(data) {
	    		deferred.resolve(data);
	    		stopLoading($rootScope);
	    	});
	    	
	    	return deferred.promise;
		}
}
