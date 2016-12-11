/**
 * This class supports validating Skeleton framework (http://getskeleton.com/)
 */
(function($) {
    FormValidation.Framework.Skeleton = function(element, options) {
        options = $.extend(true, {
            button: {
                selector: '[type="submit"]:not([formnovalidate])',
                // The class of disabled button
                disabled: 'fv-button-disabled'
            },
            err: {
                clazz: 'fv-help-block',
                parent: '^.*columns.*$'
            },
            // Skeleton doesn't support feedback icon
            icon: {
                valid: null,
                invalid: null,
                validating: null,
                feedback: 'fv-control-feedback'
            },
            row: {
                selector: '.row',
                valid: 'fv-has-success',
                invalid: 'fv-has-error',
                feedback: 'fv-has-feedback'
            }
        }, options);

        FormValidation.Base.apply(this, [element, options]);
    };

    FormValidation.Framework.Skeleton.prototype = $.extend({}, FormValidation.Base.prototype, {
        /**
         * Specific framework might need to adjust the icon position
         *
         * @param {jQuery} $field The field element
         * @param {jQuery} $icon The icon element
         */
        _fixIcon: function($field, $icon) {
            var ns      = this._namespace,
                type    = $field.attr('type'),
                field   = $field.attr('data-' + ns + '-field'),
                $row    = $field.closest(this.options.fields[field].row || this.options.row.selector),
                $parent = $field.parent();

            // Place it after the container of checkbox/radio
            // so when clicking the icon, it doesn't effect to the checkbox/radio element
            if ('checkbox' === type || 'radio' === type) {
                if ($parent.is('label')) {
                    $parent.addClass('fv-' + type);
                    $icon.insertAfter($parent);
                }
            }

            var $labels = $row.find('label');
            if ($labels.length === 0 || $labels.eq(0).is($parent)) {
                $icon.addClass('fv-icon-no-label');
            }
        }
    });
}(jQuery));