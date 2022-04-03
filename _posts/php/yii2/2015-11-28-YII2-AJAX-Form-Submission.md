---
title: "Yii2 AJAX Form Submission"
date: 2015-11-28 11:28:04
tags: php yii2
---

Yii2 provides very nice functionality to create forms and handle client and
server side validation. It creates client side validation rules automatically
from rules provided in Model class. Yii also provides its own Javascript
functionality to validate and submit forms.

<!--more-->

Yii triggers many events in Javascript that we can listen to, to modify the
default behavior according to our needs. Following is the list of events that
are currently triggered at different points.

- `beforeValidate` is triggered before validating the whole form
- `afterValidate` is triggered after validating the whole form
- `beforeValidateAttribute` is triggered before validating a single attribute of the form.
- `afterValidateAttribute` is triggered after validating a single attribute of the form.
- `beforeSubmit` is triggered before submitting the form after all validations have passed.
- `ajaxBeforeSend` is triggered before sending an AJAX request for AJAX-based validation.
- `ajaxComplete` is triggered after completing an AJAX request for AJAX-based validation.

By default, when the user submits the form, Yii will validate it and submit it
if everything is fine. But sometimes we need to submit the form with AJAX
request instead of a regular HTTP request. We can do this by listening to
`beforeSubmit` event which is triggered by Yii2 after validation and before
submitting the form.

```javascript
$("body").on("beforeSubmit", "form#formId", function () {
  var form = $(this);
  // return false if form still have some validation errors
  if (form.find(".has-error").length) {
    return false;
  }
  // submit form
  $.ajax({
    url: form.attr("action"),
    type: "post",
    data: form.serialize(),
    success: function (response) {
      // do something with response
    },
  });
  return false;
});
```

In the above example, we are attaching a `beforeSubmit` event handler to body
and providing the `form#formId` selector so that this event is triggered only
for this form. You should replace the `#formId` with the id of your form.

Then just to be sure, we are checking if form still have some validation
errors and returning `false` if it has. And lastly, we are making an actual
Ajax request to the server. In the above example, we are submitting a form to
its default action, but you can submit it to any url you want.

The important point is that, if we return `false` from event handler then form
submission will be stopped and Yii will not submit the form.

Also, When sending a response for an Ajax request in your controller action,
make sure that you have set the response type to JSON, before sending the
response, like following:

```php
$data = 'Some data to be returned in response to ajax request';
Yii::$app->response->format = \yii\web\Response::FORMAT_JSON;
return $data;
```

Yii provides support for different kinds if response types out of the box. You
can read more about these types in
[docs](http://www.yiiframework.com/doc-2.0/guide-runtime-responses.html#response-body)
