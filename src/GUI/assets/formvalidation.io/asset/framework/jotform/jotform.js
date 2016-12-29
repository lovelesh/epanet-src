(function($) {
    FormValidation.Framework.Jotform = function(element, options) {
        options = $.extend(true, {
            button: {
                selector: '[type="submit"]:not([formnovalidate])',
                // The class of disabled button
                disabled: 'fv-button-disabled'
            },
            err: {
                clazz: 'fv-help-block',
                parent: '^.*form-input-wide.*$'
            },
            // Skeleton doesn't support feedback icon
            icon: {
                valid: null,
                invalid: null,
                validating: null,
                feedback: 'fv-control-feedback'
            },
            row: {
                // By default, the field and its label are placed inside
                // a .form-line element
                selector: '.form-line',
                valid: 'fv-has-success',
                invalid: 'fv-has-error',
                feedback: 'fv-has-feedback'
            }
        }, options);

        FormValidation.Base.apply(this, [element, options]);
    };

    // The JotForm class need to extend from the FormValidation's Base one
    FormValidation.Framework.Jotform.prototype = $.extend({}, FormValidation.Base.prototype, {
    });
}(jQuery));