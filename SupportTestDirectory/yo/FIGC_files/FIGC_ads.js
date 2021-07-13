$(document).ready(function(){
    //https://developer.vimeo.com/player/sdk/reference#about-player-methods
    var admodal = $(".ad-modal.modal");
    
    if (admodal && admodal.length > 0){
        console.log("admodal exists");
        
        var cookiename = admodal.data("figc-cookiename");
        var cookieduration = admodal.data("figc-cookieduration");
        var closebuttonshowcountdown = admodal.data("figc-closebuttonshowcountdown");
        var autoclosecountdown = admodal.data("figc-autoclosecountdown");
        
        if (!cookieduration){
            cookieduration = 0;
        }
        
        if (!autoclosecountdown){
            autoclosecountdown = 20;
        }
        
        if (!getCookie(cookiename) || cookieduration == 0) {
            var videoContainer = document.querySelector('#vimeovideo');
            var unmuteButton = admodal.find(".unmutebtn");
            var muteButton = admodal.find(".mutebtn");
            
            var imageContainer = admodal.find("adBanner");
            
            var closeButton = admodal.find(".closebtn");
            var messageDiv = admodal.find(".ad-messaggio");
            var countdownSpan = admodal.find(".seconds");
            
            var openDialg = function(data) {
                admodal.modal('show');
            };
            
            var closeModalFnc = function(){
                admodal.modal('hide');
            };
            
            var showCloseBtnFnc = function(){
                closeButton.show();
            };
            
            closeButton.hide();
            
            if (videoContainer) {
            
                var vimeoOptions = {
                  id: 470203186,
                  background: 1, 
                  loop: 0, 
                  muted: 1, 
                  quality: '720p'
                };
                
                var vimeoPlayer = new Vimeo.Player(videoContainer, vimeoOptions);
    
                unmuteButton.on("click", function(){
                    unmute(vimeoPlayer, unmuteButton, muteButton);
                });
                
                admodal.on('hidden.bs.modal', function (e) {
                    pause(vimeoPlayer);
                });
                
                muteButton.on("click", function(){
                    mute(vimeoPlayer, unmuteButton, muteButton);
                });
                
                vimeoPlayer.on('bufferend', openDialg);
                
                vimeoPlayer.on('play', function(data) {
                    unmuteButton.show();
                    messageDiv.show();
                    countdownSpan.text(autoclosecountdown);
                    //console.log(data.duration);
                    startCountdown(null, closebuttonshowcountdown, showCloseBtnFnc);
                    startCountdown(countdownSpan, autoclosecountdown, closeModalFnc);
    
                });
                
                vimeoPlayer.on('ended', function(data) {
                    //console.log("finito");
                    if (cookieduration > 0){
                        setCookie(cookiename,true,cookieduration);
                    }
                    unmuteButton.hide();
                    muteButton.hide();
                    closeButton.show();
                });
            
            }
            else {
                console.log("apertura admodal")
                openDialg();
                messageDiv.show();
                //closeButton.show();
                startCountdown(null, closebuttonshowcountdown, showCloseBtnFnc);
                startCountdown(countdownSpan, autoclosecountdown, closeModalFnc);
            }
        }
    
    }

});

function mute(vPlayer, unmuteButton, muteButton){
    unmuteButton.show();
    muteButton.hide();
    
    vPlayer.setVolume(0).then(function(volume) {
          // The volume is set
        }).catch(function(error) {
          switch (error.name) {
            case 'RangeError':
                // The volume is less than 0 or greater than 1
                break;
        
            default:
                console.log(error.name);
                break;
          }
    });            
}

function unmute(vPlayer, unmuteButton, muteButton){
    unmuteButton.hide();
    muteButton.show();
    
    vPlayer.setVolume(1).then(function(volume) {
          // The volume is set
        }).catch(function(error) {
          switch (error.name) {
            case 'RangeError':
                // The volume is less than 0 or greater than 1
                break;
        
            default:
                console.log(error.name);
                break;
          }
    });      
    
}

function startCountdown(countdownSpan, interval, func){
    var adesso = new Date().getTime() / 1000;
    //console.log(now);
    //var countdownDate = now.getSeconds() + interval;
    var countdownDate = adesso + interval;
    
    //console.log("adesso:" + adesso);
    //console.log("countdownDate: " + countdownDate);
    
    var x = setInterval(function(){
        var ora = new Date().getTime() / 1000;
        var distance = Math.trunc(countdownDate - ora);
        
        //console.log("remaining: " + distance);
        if (countdownSpan) {
            countdownSpan.text(distance);
        }
        
        if (distance < 0) {
            clearInterval(x);
            func();
          }
    }, 100);
    
}

	// Helper scripts
	// Get cookie
	var getCookie = function(name) {
		var nameEQ = name + "=";
		var ca = document.cookie.split(';');
		for(var i=0;i < ca.length;i++) {
			var c = ca[i];
			while (c.charAt(0)==' ') c = c.substring(1,c.length);
			if (c.indexOf(nameEQ) === 0) return c.substring(nameEQ.length,c.length);
		}
		return null;
	};
	
	// Set cookie
	var setCookie = function(name,value,days) {
		var expires = "";
		if (days) {
			var date = new Date();
			date.setTime(date.getTime()+(days*24*60*60*1000));
			expires = "; expires="+date.toGMTString();
		}
		document.cookie = name+"="+value+expires+"; path=/";
	};
	