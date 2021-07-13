
$(function () {
    // selettore di dimensione pagina
    $("#pageSizeList").on("change", function(){
        var form = $("#figcsearchform");
        $("#figcpagesize").val($(this).val());
        form.submit();
    });    
    
    // Menu mobile
    if (isMenuMobileVisible()){
        loadMenuMobileHtml();
    }
    else{
        var jMenuButton = $("#nav-side-menu-button");
        jMenuButton.on("click", function() {
            loadMenuMobileHtml();
        });            
    }
    
    // TIMELINE TRIONFI AZZURRI 
    
    $('.bloccoTimeline').horizontalTimeline({
        useScrollBtns: false,
		addRequiredFile: false,
		useFontAwesomeIcons: false,
		useTouchSwipe: false
    });
    
    // fix menu superiore al caricamento
    updateNav();
    // caricamento immagini in viewport
    loadLazyImages();
    
    // DatePicker
    $('.figcDatePicker').datetimepicker({
        format: 'L',
        locale: 'it'
    });

    $(window).on('resize scroll', function() {
        loadLazyImages();
        
        var jSocialWall = $(".socialWallFigc");
       
       if (jSocialWall.data("figc-socialwall")) {
    
           if (isInViewport(jSocialWall)) {
            if (jSocialWall.data("figc-socialwall")) {

               var socialwallQry = jSocialWall.data("figc-socialwall");
               jSocialWall.data("figc-socialwall", null);
               
               jSocialWall.load("/utility/socialwall/?" + socialwallQry);
            }
           }
       }
        /*
       var jSocialWall = $(".socialWallFigc");
       
       if (jSocialWall.data("figc-socialwall")) {
    
           if (isInViewport(jSocialWall)) {
            if (jSocialWall.data("figc-socialwall")) {

               var socialwallQry = jSocialWall.data("figc-socialwall");
               jSocialWall.removeData("figc-socialwall");
               
               jSocialWall.load("/utility/socialwall/?" + socialwallQry);
            }
           }
       }
       
       */

    });
    
    // fix pausa sui caroselli video
    $(".carousel-video").on('slide.bs.carousel', function (event) {
            var c = $(this);
            var citems = c.find(".carousel-item");
            var fromItem = citems.eq(event.from);
            var v = fromItem.find("video");
            if (v.length > 0) {
                v.get(0).pause();
            }
            else { 
                var ifr = fromItem.find("iframe");

                if (ifr.length > 0) {
                    ifr[0].contentWindow.postMessage('{"event":"command","func":"' + 'pauseVideo' + '","args":""}', '*');
                }
            }
           
        });
    
    // SLIDER WIDGET E-COMMERCE
        var slider = $("#light-slider").lightSlider({
        item: 4,
        controls: false,
        auto: false,
        loop: true,
        slideMove: 2,
        responsive: [
            {
                breakpoint: 1200,
                settings: {
                    item: 3,
                    slideMove: 2,
                    slideMargin: 6,
                }
            },
            {
                breakpoint: 991,
                settings: {
                    item: 2,
                    slideMove: 1
                }
            },
            {
                breakpoint: 768,
                settings: {
                    item: 1,
                    slideMove: 1
                }
            }
        ]

    });
    $('.slideControls .slidePrev').click(function () {
        slider.goToPrevSlide();
    });

    $('.slideControls .slideNext').click(function () {
        slider.goToNextSlide();
    });
    
    
});

function isInViewport(jEl) {
    var elementTop = jEl.offset().top - 0;
    var elementBottom = elementTop + jEl.outerHeight();

    var viewportTop = $(window).scrollTop();
    var viewportBottom = viewportTop + $(window).height();

    return elementBottom > viewportTop && elementTop < viewportBottom;
}

function loadLazyImages() {
   var jImgLazy = $("img[data-figc-src]");
   
   jImgLazy.each(function(index, item) {
       var jItem = $(item);
       
       if (jItem.data("figc-src") && isInViewport(jItem)) {
            jItem.attr("src", jItem.data("figc-src"));
            jItem.removeData("figc-src");
       }
   });
}

// NUOVO MENU TOP 
        var $nav = $('.greedy-nav');
        var $btn = $('.greedy-nav button');
        var $vlinks = $('.greedy-nav .visible-links');
        var $hlinks = $('.greedy-nav .hidden-links');

        var breaks = [];

        function updateNav() {

            var availableSpace = $btn.hasClass('hidden') ? $nav.width() : $nav.width() - $btn.width() - 30;

            // The visible list is overflowing the nav
            if ($vlinks.width() > availableSpace) {

                // Record the width of the list
                breaks.push($vlinks.width());

                // Move item to the hidden list
                $vlinks.children().last().prependTo($hlinks);

                // Show the dropdown btn
                if ($btn.hasClass('hidden')) {
                    $btn.removeClass('hidden');
                }

                // The visible list is not overflowing
            } else {

                // There is space for another item in the nav
                if (availableSpace > breaks[breaks.length - 1]) {

                    // Move the item to the visible list
                    $hlinks.children().first().appendTo($vlinks);
                    breaks.pop();
                }

                // Hide the dropdown btn if hidden list is empty
                if (breaks.length < 1) {
                    $btn.addClass('hidden');
                    $hlinks.addClass('hidden');
                }
            }

            // Keep counter updated
            $btn.attr("count", breaks.length);

            // Recur if the visible list is still overflowing the nav
            if ($vlinks.width() > availableSpace) {
                updateNav();
            }

        }

        // Window listeners

        $(window).resize(function () {
            updateNav();
        });

        $btn.on('click', function () {
            $hlinks.toggleClass('hidden');
        });

        //updateNav();

    function loadMenuMobileHtml(){
        var jMenuButton = $("#nav-side-menu-button");
        var currentNodeId = jMenuButton.data("figc-currentnode");
        
        console.log('currentNodeId is ', currentNodeId);
        
        if (currentNodeId){
            jMenuButton.data("figc-currentnode", null);
            var jPlaceholder = $("#menu-content");
            jPlaceholder.load("/utility/menumobile?rootnodeid=" + currentNodeId);
        }
        
    }

    function isMenuMobileVisible() {
        return $("#nav-side-menu-button").is(":visible");
    }

//Mediagallery script

$('#default-demo').slickLightbox({
    closeOnBackdropClick : false,
    itemSelector : '.thumbnail',
    caption: 'caption',
    slick : function ($e) {

      $e.find('.slick-lightbox-slick-iframe').each(function () {
        $(this)
          .attr('data-src', $(this).attr('src'))
          .attr('src', '')
      })

      function clearIframe (slick, index) {
        var $iframe = $(slick.$slides.get(index)).find('.slick-lightbox-slick-iframe')
        if ($iframe.length) {
          setTimeout(function () {
            $iframe.attr('src', '')
          }, slick.options.speed)
        }
      }

      function loadIframe (slick, index) {
        var $iframe = $(slick.$slides.get(index)).find('.slick-lightbox-slick-iframe')
        if ($iframe.length) $iframe.attr('src', $iframe.attr('data-src'))
      }

      /**
       * Return slick instance
       */
      return $e.find('.slick-lightbox-slick')
        .on('init', function (event, slick) {
          loadIframe(slick, slick.currentSlide)
        })
        .on('beforeChange', function (event, slick, currentSlide, nextSlide) {
          clearIframe(slick, currentSlide)
          loadIframe(slick, nextSlide)
        })
        .slick()
    }
  });




//DATEPICKER
/*
$(function () {
    $('#datetimepicker1').datetimepicker({
        format: 'L',
        locale: 'it'
    });
    $('#datetimepicker2').datetimepicker({
        format: 'L',
        locale: 'it'
    });
});

*/

// SLIDER WIDGET E-COMMERCE
/*
$(document).ready(function () {
    var slider = $("#light-slider").lightSlider({
        item: 4,
        controls: false,
        auto: false,
        loop: true,
        slideMove: 2,
        responsive: [
            {
                breakpoint: 1200,
                settings: {
                    item: 3,
                    slideMove: 2,
                    slideMargin: 6,
                }
            },
            {
                breakpoint: 991,
                settings: {
                    item: 2,
                    slideMove: 1
                }
            },
            {
                breakpoint: 768,
                settings: {
                    item: 1,
                    slideMove: 1
                }
            }
        ]

    });
    $('.slideControls .slidePrev').click(function () {
        slider.goToPrevSlide();
    });

    $('.slideControls .slideNext').click(function () {
        slider.goToNextSlide();
    });
});
*/

// SLIDER WIDGET E-COMMERCE 2

$(document).ready(function () {
    
    $('.pollContainer .form-group .card').click(function () {
    $('.pollContainer .form-group .card').removeClass('selected');
    $(this).addClass('selected');
    var jAnswerId =$(this).data('answer');
    $('#selectedAnswer').val(jAnswerId);
    });
    
    
    
    var slider = $("#light-slider-eCommerce2").lightSlider({
        item: 3,
        controls: false,
        auto: false,
        loop: true,
        slideMove: 2,
        responsive: [
            {
                breakpoint: 1200,
                settings: {
                    item: 3,
                    slideMove: 2,
                    slideMargin: 6,
                }
            },
            {
                breakpoint: 991,
                settings: {
                    item: 2,
                    slideMove: 1
                }
            },
            {
                breakpoint: 768,
                settings: {
                    item: 1,
                    slideMove: 1
                }
            }
        ]

    });
    $('.slideControls .slidePrev').click(function () {
        slider.goToPrevSlide();
    });

    $('.slideControls .slideNext').click(function () {
        slider.goToNextSlide();
    });
});




// SLIDER WIDGET I NOSTRI CALCIATORI

$(document).ready(function () {
    var slider = $("#light-slider-calciatori").lightSlider({
        item:4,
        controls: true,
        auto: false,
        loop: true,
        responsive: [
            {
                breakpoint: 1186,
                settings: {
                    item: 3,
                    slideMove: 1,
                    
                }
            },
            {
                breakpoint: 976,
                settings: {
                    item: 3,
                    slideMove: 1
                }
            },
            {
                breakpoint: 768,
                settings: {
                    item: 4,
                    slideMove: 1
                }
            },
             {
                breakpoint: 650,
                settings: {
                    item: 3,
                    slideMove: 1
                }
            },
            {
                breakpoint: 470,
                settings: {
                    item: 2,
                    slideMove: 1
                }},
            {
                breakpoint: 300,
                settings: {
                    item: 1,
                    slideMove: 1
                }
            }
        ]

    });
    $('.slideControls-calciatori .slidePrev').click(function () {
        slider.goToPrevSlide();
    });

    $('.slideControls-calciatori .slideNext').click(function () {
        slider.goToNextSlide();
    });
});


// SLIDER WIDGET I NOSTRI CALCIATORI COL8

$(document).ready(function () {
    var slider = $("#light-slider-calciatori-col8").lightSlider({
        item:4,
        controls: true,
        auto: false,
        loop: true,
        responsive: [
            {
                breakpoint: 1186,
                settings: {
                    item: 4,
                    slideMove: 1,
                    
                }
            },
            {
                breakpoint: 976,
                settings: {
                    item: 2,
                    slideMove: 1
                }
            },
            {
                breakpoint: 750,
                settings: {
                    item: 3,
                    slideMove: 1
                }
            },
            {
                breakpoint: 470,
                settings: {
                    item: 2,
                    slideMove: 1
                }
            }
        ]

    });
    $('.slideControls-calciatori .slidePrev').click(function () {
        slider.goToPrevSlide();
    });

    $('.slideControls-calciatori .slideNext').click(function () {
        slider.goToNextSlide();
    });
});




// SLIDER WIDGET TECNICI DELLE SQUADRE NAZIONALI 

$(document).ready(function () {
    var jSlider = $("#light-slider-tecnici")[0];
    if(jSlider)
    {
        var jItemsNumber = $("#light-slider-tecnici")[0].getAttribute("data-figc-items");
        var jItemsBreakpoint_1200 = 4;
        var jItemsBreakpoint_992 = 3;
        
        if (!jItemsNumber) {
            jItemsNumber = 5;
        }
        
        if(jItemsNumber == 3) {
            jItemsBreakpoint_1200 = 2;
            jItemsBreakpoint_992 = 2
            
        }
        
        var slider = $("#light-slider-tecnici").lightSlider({
            item: jItemsNumber,
            controls: true,
            auto: false,
            loop: true,
            responsive: [
                {
                    breakpoint: 1200,
                    settings: {
                    item: jItemsBreakpoint_1200,
                    slideMove: 1,
                        
                    }
                },
                {
                    breakpoint: 992,
                    settings: {
                    item: jItemsBreakpoint_992,
                    slideMove: 1
                    }
                },
                {
                    breakpoint: 768,
                    settings: {
                        item: 2,
                        slideMove: 1
                    }
                },
                {
                    breakpoint: 470,
                    settings: {
                        item: 1,
                        slideMove: 1
                    }
                }
            ]
    
        });
        $('.slideControls-tecnici .slidePrev').click(function () {
            slider.goToPrevSlide();
        });
    
        $('.slideControls-tecnici .slideNext').click(function () {
            slider.goToNextSlide();
        });
    }
});


// SLIDER WIDGET CONTENT INT CAROUSEL

$(document).ready(function () {
    var slider = $(".contentInt-carousel").lightSlider({
        item:5,
        controls: true,
        auto: false,
        loop: true,
        responsive: [
            {
                breakpoint: 1186,
                settings: {
                    item: 3,
                    slideMove: 1,
                    
                }
            },
            {
                breakpoint: 976,
                settings: {
                    item: 2,
                    slideMove: 1
                }
            },
            {
                breakpoint: 750,
                settings: {
                    item: 3,
                    slideMove: 1
                }
            },
            {
                breakpoint: 470,
                settings: {
                    item: 2,
                    slideMove: 1
                }
            }
        ]

    });
    $('.slideControls-contInt-carousel .slidePrev').click(function () {
        slider.goToPrevSlide();
    });

    $('.slideControls-contInt-carousel .slideNext').click(function () {
        slider.goToNextSlide();
    });
});


// SLIDER WIDGET MEDIAGALLERY2

$(document).ready(function () {
    $('#vertical').lightSlider({
        gallery: true,
        item: 1,
        vertical: true,
        verticalHeight: 320,
        vThumbWidth: 120,
        thumbItem: 4,
        thumbMargin: 4,
        slideMargin: 0
    });
});




// COUNTER

(function ($) {
    $.fn.countTo = function (options) {
        options = options || {};

        return $(this).each(function () {
            // set options for current element
            var settings = $.extend({}, $.fn.countTo.defaults, {
                from: $(this).data('from'),
                to: $(this).data('to'),
                speed: $(this).data('speed'),
                refreshInterval: $(this).data('refresh-interval'),
                decimals: $(this).data('decimals')
            }, options);

            // how many times to update the value, and how much to increment the value on each update
            var loops = Math.ceil(settings.speed / settings.refreshInterval),
                increment = (settings.to - settings.from) / loops;

            // references & variables that will change with each update
            var self = this,
                $self = $(this),
                loopCount = 0,
                value = settings.from,
                data = $self.data('countTo') || {};

            $self.data('countTo', data);

            // if an existing interval can be found, clear it first
            if (data.interval) {
                clearInterval(data.interval);
            }
            data.interval = setInterval(updateTimer, settings.refreshInterval);

            // initialize the element with the starting value
            render(value);

            function updateTimer() {
                value += increment;
                loopCount++;

                render(value);

                if (typeof (settings.onUpdate) == 'function') {
                    settings.onUpdate.call(self, value);
                }

                if (loopCount >= loops) {
                    // remove the interval
                    $self.removeData('countTo');
                    clearInterval(data.interval);
                    value = settings.to;

                    if (typeof (settings.onComplete) == 'function') {
                        settings.onComplete.call(self, value);
                    }
                }
            }

            function render(value) {
                var formattedValue = settings.formatter.call(self, value, settings);
                $self.html(formattedValue);
            }
        });
    };

    $.fn.countTo.defaults = {
        from: 0,               // the number the element should start at
        to: 0,                 // the number the element should end at
        speed: 1000,           // how long it should take to count between the target numbers
        refreshInterval: 100,  // how often the element should be updated
        decimals: 0,           // the number of decimal places to show
        formatter: formatter,  // handler for formatting the value before rendering
        onUpdate: null,        // callback method for every time the element is updated
        onComplete: null       // callback method for when the element finishes updating
    };

    function formatter(value, settings) {
        return value.toFixed(settings.decimals);
    }
}(jQuery));

jQuery(function ($) {
    // custom formatting example
    $('.count-number').data('countToOptions', {
        formatter: function (value, options) {
            return value.toFixed(options.decimals).replace(/\B(?=(?:\d{3})+(?!\d))/g, ',');
        }
    });

    // start all the timers
    $('.timer').each(count);

    function count(options) {
        var $this = $(this);
        options = $.extend({}, options || {}, $this.data('countToOptions') || {});
        $this.countTo(options);
    }
});


// CAROSELLO BOOTSTRAP USATO NELLE APERTURE E NEL WIDGET VIDEO

$(function () {
    var jCarousel = $('.carousel');


    jCarousel.on('slide.bs.carousel', function (e) {
        var jElements = $(e.target).find('.post');
        var jToBeActivated = $(jElements.get(e.to));

        jElements.removeClass('active');

        jToBeActivated.addClass('active');
    });
});


// CAROSELLO lightSlider USATO NELLE APERTURE DELLE HOME DI SEZIONE

$(document).ready(function () {
    $('#vertical-carousel').lightSlider({
        gallery: true,
        item: 1,
        vertical: true,
        verticalHeight: 550,
        vThumbWidth: 250,
        thumbItem: 4,
        thumbMargin: 4,
        slideMargin: 0
    });
});


