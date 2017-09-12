$('a.post').click(function(){
    $.post($(this).attr('href'));
	return false;
});

