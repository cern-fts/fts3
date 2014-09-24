
// Overview

function StatsOverviewCtrl($routeParams, $location, $scope, stats, Statistics, Unique)
{
	$scope.stats = stats;
	$scope.host = $location.$$search.hostname;

	$scope.hostnames = Unique('hostnames')

	$scope.filterHost = function(host) {
		var filter = $location.$$search;
		if (host)
			filter.hostname = host;
		else
			delete filter.hostname;
		$location.search(filter);
		$scope.host = host;
	}

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.$$search;
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

    	Statistics.query($location.$$search,
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));

    	return deferred.promise;
	}
}

// Per server

function StatsServersCtrl($location, $scope, servers, Servers)
{
	$scope.servers = servers;

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.$$search;
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

    	Servers.query($location.$$search,
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));

    	return deferred.promise;
	}
}

// Per VO

function StatsVosCtrl($location, $scope, vos, StatsVO, Unique)
{
	$scope.vos = vos;

	// Filter
	$scope.unique = {
		sources: Unique('sources'),
		destinations: Unique('destinations')
	}

	$scope.filter = {
		'source_se': validString($location.$$search.source_se),
		'dest_se':   validString($location.$$search.dest_se),
	}

	$scope.applyFilters = function() {
		$location.search($scope.filter);
	}

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.$$search;
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

    	StatsVO.query($location.$$search,
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));

    	return deferred.promise;
	}
}

// Transfer volume
function TransferVolumeCtrl($location, $scope, volumes)
{
    $scope.volumes = volumes;

    // On page change, reload
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };
}

TransferVolumeCtrl.resolve = {
    volumes: function($rootScope, $location, $q, TransferVolume) {
        loading($rootScope);

        var deferred = $q.defer();

        TransferVolume.query($location.$$search,
                genericSuccessMethod(deferred, $rootScope),
                genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}

// Profiling

function StatsProfilingCtrl($location, $scope, profile, Profile)
{
	$scope.profile = profile;

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.$$search;
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

    	Profile.query($location.$$search,
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

    	SlowQueries.query($location.$$search,
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));

    	return deferred.promise;
	}
}

// TURLS
function TurlsCtrl($location, $scope, turls)
{
    $scope.turls = turls;

    // On page change, reload
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };

	$scope.filterBy = function(filter) {
		$location.search(filter);
	}
}

TurlsCtrl.resolve = {
    turls: function($rootScope, $location, $q, Turls) {
        loading($rootScope);

        var deferred = $q.defer();

        Turls.query($location.$$search,
            genericSuccessMethod(deferred, $rootScope),
            genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}
