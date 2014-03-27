
function StatsOverviewCtrl($routeParams, $location, $scope, stats, Statistics, Unique)
{
	$scope.stats = stats;
	$scope.host = $location.search().hostname;
	
	$scope.hostnames = Unique('hostnames')
	
	$scope.filterHost = function(host) {
		var filter = $location.search();
		if (host)
			filter.hostname = host;
		else
			delete filter.hostname;
		$location.search(filter);
		$scope.host = host;
	}
	
	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
    	$scope.stats = Statistics.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


StatsOverviewCtrl.resolve = {
	stats: function($route, $rootScope, $location, $q, Statistics) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	Statistics.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));
    	
    	return deferred.promise;
	}
}


function StatsServersCtrl($location, $scope, servers, Servers)
{
	$scope.servers = servers;
	
	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
    	Servers.query(filter, function (updatedServers) {
            for(var server in updatedServers) {
            	if (server.toString().substring(0, 1) != '$')
            		updatedServers[server].show = $scope.servers[server].show;
            }
            $scope.servers = updatedServers;
        });
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


StatsServersCtrl.resolve = {
	servers: function($rootScope, $location, $q, Servers) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var page = $location.search().page;
    	if (!page || page < 1)
    		page = 1;
    	
    	Servers.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));
    	
    	return deferred.promise;
	}
}


function StatsVosCtrl($location, $scope, vos, StatsVO, Unique)
{
	$scope.vos = vos;
	
	// Filter
	$scope.unique = {
		sources: Unique('sources'),
		destinations: Unique('destinations')
	}
	
	$scope.filter = {
		'source_se': validString($location.search().source_se),
		'dest_se':   validString($location.search().dest_se),
	}
	
	$scope.applyFilters = function() {
		$location.search($scope.filter);
	}

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
    	$scope.vos = StatsVO.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


StatsVosCtrl.resolve = {
	vos: function($rootScope, $location, $q, StatsVO) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var page = $location.search().page;
    	if (!page || page < 1)
    		page = 1;
    	
    	StatsVO.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));
    	
    	return deferred.promise;
	}
}


function StatsProfilingCtrl($location, $scope, profile, Profile)
{
	$scope.profile = profile;

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
    	$scope.profile = Profile.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


StatsProfilingCtrl.resolve = {
	profile: function($rootScope, $location, $q, Profile) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var page = $location.search().page;
    	if (!page || page < 1)
    		page = 1;
    	
    	Profile.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));
    	
    	return deferred.promise;
	}
}


function SlowQueriesCtrl($location, $scope, slowQueries)
{
	$scope.slowQueries = slowQueries;
}

SlowQueriesCtrl.resolve = {
	slowQueries: function($rootScope, $location, $q, SlowQueries) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var page = $location.search().page;
    	if (!page || page < 1)
    		page = 1;
    	
    	SlowQueries.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));
    	
    	return deferred.promise;
	}
}