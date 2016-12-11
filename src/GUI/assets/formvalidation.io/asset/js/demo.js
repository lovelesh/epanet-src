$(document).ready(function() {
    $('form').on('success.form.fv', function(e) {
        e.preventDefault();
    });

    $(window).on('load', function() {
        var $parentWindow = $(document, window.parent.document);
        if ($parentWindow.length) {
            var fullUrl   = $parentWindow.eq(0).context.baseURI,
                parentUrl = window.parent.document.location.origin,
                path      = fullUrl.substr(parentUrl.length),
                $iframe   = $(window.parent.document).find('iframe[src="' + path + '"]');

            if ($iframe.length) {
                //$('body').addClass('demo-frame-body').find('.demo-ad').hide();

                // Adjust the height of iframes containing the demo for specific frameworks
                var $container = $('#demoContainer');
                setTimeout(function() {
                    $iframe.height($container.height()).css('visibility', 'visible').fadeIn('fast');
                }, 1000);

                // Set the internal data to get it later if I want
                $('body').data('iframe.fv', $iframe);

                $container
                    .find('form')
                    .on('status.field.fv', function(e, data) {
                        $iframe.height($container.height());
                    });
            }
        }
    });
});