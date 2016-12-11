/**
 * This class supports validating Spectre framework (https://picturepan2.github.io/spectre/)
 */
(function($) {
    FormValidation.Framework.Spectre = function(element, options) {
        options = $.extend(true, {
            button: {
                selector: '[type="submit"]:not([formnovalidate])',
                // The class of disabled button
                disabled: 'fv-button-disabled'
            },
            err: {
                clazz: 'fv-help-block',
                parent: '^(.*)col-(xs|sm|md)-[0-9]+(.*)$'
            },
            // Spectre doesn't support feedback icon
            icon: {
                valid: null,
                invalid: null,
                validating: null,
                feedback: 'fv-control-feedback'
            },
            row: {
                selector: '.form-group',
                valid: 'has-success',
                invalid: 'has-danger',
                feedback: 'fv-has-feedback'
            }
        }, options);

        FormValidation.Base.apply(this, [element, options]);
    };

    FormValidation.Framework.Spectre.prototype = $.extend({}, FormValidation.Base.prototype, {
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
        }
    });
}(jQuery));