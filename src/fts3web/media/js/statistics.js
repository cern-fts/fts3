
function StatsOverviewCtrl($location, $scope, stats, Statistics)
{
	$scope.stats = stats;
	
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


function StatsServersCtrl($location, $scope, servers, Servers)
{
	$scope.servers = servers;
	
	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
    	$scope.servers = Servers.query(filter);
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
    	
    	Servers.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
    	return deferred.promise;
	}
}


function StatsPairsCtrl($location, $scope, pairs, Pairs)
{
	$scope.pairs = pairs;
	
	// Filter
	$scope.filterSource = function(source) {
		var filter = $location.search();
		filter.source_se = source;
		$location.search(filter);
	}
	$scope.filterDestination = function(destination) {
		var filter = $location.search();
		filter.dest_se = destination;
		$location.search(filter);
	}
	
	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
    	$scope.pairs = Pairs.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


StatsPairsCtrl.resolve = {
	pairs: function($rootScope, $location, $q, Pairs) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var page = $location.search().page;
    	if (!page || page < 1)
    		page = 1;
    	
    	Pairs.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
    	return deferred.promise;
	}
}


function StatsVosCtrl($location, $scope, vos, StatsVO)
{
	$scope.vos = vos;

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
    	
    	StatsVO.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
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
    	
    	Profile.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
    	return deferred.promise;
	}
}
