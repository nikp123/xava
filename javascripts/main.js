function GetLatestReleaseInfo() {
	$.getJSON("https://api.github.com/repos/nikp123/xava/releases").done(function(release) {
		var foundLatestRelease = false;

		$(".changelog").fadeIn("slow");
		var md = new Remarkable();
		var releaseCount = release.length;
		if(releaseCount > 5) releaseCount = 5;
		for(var i = 0; i < releaseCount; i++) {
			if(foundLatestRelease == false && release[i].prerelease == false) {
				foundLatestRelease = true;
				$(".download").attr("href", release[i].assets[0].browser_download_url);
			}
			var oneHour = 60 * 60 * 1000;
			var oneDay = 24 * oneHour;
			var dateDiff = new Date() - new Date(release[i].published_at);
			var timeAgo;
			if (dateDiff < oneDay) timeAgo = (dateDiff / oneHour).toFixed(1) + " hours ago";
			else timeAgo = (dateDiff / oneDay).toFixed(1) + " days ago";

			var releaseHTML = "";
			releaseHTML += "<h3>" + release[i].tag_name + " " + release[i].name + " (" + timeAgo + ")";
			if(release[i].prerelease == true) releaseHTML += " (pre-release)";
			releaseHTML += "</h3>";
			releaseHTML += md.render(release[i].body);
			for (var j = 0; j < release[i].assets.length; j++) {
				releaseHTML += "<a href=\""  + "\">Download " + release[i].assets[j].name + "</a> (downloads " + release[i].assets[j].download_count + ")<br>";
			}
			releaseHTML += "<a href=\"" + release[i].tarball_url + "\">Download as tarball</a><br>";
			releaseHTML += "<a href=\"" + release[i].zipball_url + "\">Download as .zip</a><br>";
			releaseHTML+="<br>";
			$(".changelog").append(releaseHTML);
		}
	});
}

GetLatestReleaseInfo();


